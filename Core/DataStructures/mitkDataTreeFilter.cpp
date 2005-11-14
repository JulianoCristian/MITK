#include <itkCommand.h>
#include <mitkDataTreeFilter.h>
#include <mitkDataTreeFilterEvents.h>
#include <mitkDataTree.h>
#include <mitkPropertyManager.h>

namespace mitk
{

//------ Some common filter functions ----------------------------------------------------

/// default filter, lets everything except NULL pointers pass
static bool IsDataTreeNode(mitk::DataTreeNode* node)
{
  return ( node!= 0 );
}

//------ BasePropertyAccessor ------------------------------------------------------------
  
DataTreeFilter::BasePropertyAccessor::BasePropertyAccessor(mitk::BaseProperty* property, bool editable)
: m_Editable(editable),
  m_Property(property)
{
  // nothing to do
}

bool DataTreeFilter::BasePropertyAccessor::IsEditable() const
{
  return m_Editable;
}

DataTreeFilter::BasePropertyAccessor::operator const mitk::BaseProperty*()
{
  // const access always permitted
  return m_Property;
}

DataTreeFilter::BasePropertyAccessor::operator mitk::BaseProperty*()
{
  // non-const access only permitted when editable
  if (!m_Editable) throw NoPermissionException();
  return m_Property;
}

DataTreeFilter::BasePropertyAccessor::operator const std::string()
{
  if (!m_Property) 
    return std::string("[no value]");
  else
    return m_Property->GetValueAsString();
}

//------ Item ----------------------------------------------------------------------------
DataTreeFilter::Item::Pointer DataTreeFilter::Item::New(mitk::DataTreeNode* node, DataTreeFilter* treefilter, 
                                                        int index, const Item* parent)
{
  // from itkNewMacro()
  Pointer smartPtr;
  Item* rawPtr = new Item(node, treefilter, index, parent);
  smartPtr = rawPtr;
  rawPtr->UnRegister();
  treefilter->m_Item[node] = rawPtr;
  return smartPtr;
}

DataTreeFilter::Item::Item(mitk::DataTreeNode* node, DataTreeFilter* treefilter, 
                           int index, const Item* parent)
: m_Index(index),
  m_Parent(parent),
  m_Children(NULL),
  m_TreeFilter(treefilter),
  m_Node(node),
  m_Selected(false)
{
  m_Children = DataTreeFilter::ItemList::New();
}

DataTreeFilter::Item::~Item()
{
  // remove selection
  m_TreeFilter->m_SelectedItems.erase(this);
  // remove map entry
  m_TreeFilter->m_Item.erase(m_Node);
}

DataTreeFilter::BasePropertyAccessor DataTreeFilter::Item::GetProperty(const std::string& key) const
{
  mitk::BaseProperty* prop = m_Node->GetProperty(key.c_str(), m_TreeFilter->m_Renderer);
  if ( std::find( m_TreeFilter->m_VisibleProperties.begin(), m_TreeFilter->m_VisibleProperties.end(), key ) 
       == m_TreeFilter->m_VisibleProperties.end() )
  {
    // key not marked visible
    return BasePropertyAccessor(0, false);
  }

  if ( !prop ) // property not found in list (or "disabled")
  {
    mitk::BaseProperty::Pointer defaultProp = // create a default property
      mitk::PropertyManager::GetInstance()->CreateDefaultProperty(key.c_str());

    if ( defaultProp.IsNotNull() )
    {
      m_Node->SetProperty( key.c_str(), defaultProp );
      prop = defaultProp;
    }
  }

  // visible. determine whether property may be edited
  bool editable = 
    std::find(m_TreeFilter->m_EditableProperties.begin(), m_TreeFilter->m_EditableProperties.end(), key)
    != m_TreeFilter->m_EditableProperties.end();
  return BasePropertyAccessor(prop, editable); 
}

const DataTreeFilter::ItemList* DataTreeFilter::Item::GetChildren() const
{
  return m_Children;
}

bool DataTreeFilter::Item::HasChildren() const
{
  return m_Children->Size() > 0;
}

int DataTreeFilter::Item::GetIndex() const
{
  return m_Index;
}

bool DataTreeFilter::Item::IsRoot() const
{
  return m_Parent == NULL;
}

bool DataTreeFilter::Item::IsSelected() const
{
  return m_Selected;
}

void DataTreeFilter::Item::SetSelected(bool selected) 
{
  if ( selected != m_Selected )
  {
    m_Selected = selected;

    if ( selected )
    {
      if ( m_TreeFilter->m_SelectionMode == mitk::DataTreeFilter::SINGLE_SELECT )
      { // deselect last selected item
        m_TreeFilter->m_SelectedItems.erase( m_TreeFilter->m_LastSelectedItem.GetPointer() );
      }
      
      if ( m_TreeFilter->m_LastSelectedItem.IsNotNull() ) // remember this item as most recently selected
        m_TreeFilter->m_LastSelectedItem = this;

      m_TreeFilter->m_SelectedItems.insert(this);
    }
    else
    {
      m_TreeFilter->m_SelectedItems.erase(this);
    }
  }
}

const DataTreeFilter::Item* DataTreeFilter::Item::GetParent() const
{
  return m_Parent;
}

//------ DataTreeFilter ------------------------------------------------------------------

// smart pointer constructor
DataTreeFilter::Pointer DataTreeFilter::New(mitk::DataTree* datatree)
{
  // from itkNewMacro()
  Pointer smartPtr;
  DataTreeFilter* rawPtr = new DataTreeFilter(datatree);
  smartPtr = rawPtr;
  rawPtr->UnRegister();
  return smartPtr;
}

// real constructor (protected)
DataTreeFilter::DataTreeFilter(mitk::DataTree* datatree)
: m_Filter(NULL),
  m_DataTree(datatree),
  m_HierarchyHandling(mitk::DataTreeFilter::PRESERVE_HIERARCHY),
  m_SelectionMode(mitk::DataTreeFilter::MULTI_SELECT),
  m_Renderer(NULL),
  m_TreeNodeChangeConnection(0),
  m_TreeAddConnection(0),
  m_TreePruneConnection(0),
  m_TreeRemoveConnection(0)
{
  SetFilter( &mitk::IsDataTreeNode );
  m_Items = ItemList::New(); // create an empty list

  // connect tree notifications to member functions
  {
  itk::ReceptorMemberCommand<mitk::DataTreeFilter>::Pointer command = itk::ReceptorMemberCommand<mitk::DataTreeFilter>::New();
  command->SetCallbackFunction(this, &DataTreeFilter::TreeNodeChange);
  m_TreeNodeChangeConnection = m_DataTree->AddObserver(itk::TreeNodeChangeEvent<mitk::DataTreeBase>(), command);
  }

  {
  itk::ReceptorMemberCommand<mitk::DataTreeFilter>::Pointer command = itk::ReceptorMemberCommand<mitk::DataTreeFilter>::New();
  command->SetCallbackFunction(this, &DataTreeFilter::TreeAdd);
  m_TreeAddConnection = m_DataTree->AddObserver(itk::TreeAddEvent<mitk::DataTreeBase>(), command);
  }
  
  {
  itk::ReceptorMemberCommand<mitk::DataTreeFilter>::Pointer command = itk::ReceptorMemberCommand<mitk::DataTreeFilter>::New();
  command->SetCallbackFunction(this, &DataTreeFilter::TreeRemove);
  m_TreeRemoveConnection = m_DataTree->AddObserver(itk::TreeRemoveEvent<mitk::DataTreeBase>(), command);
  }

  {
  itk::ReceptorMemberCommand<mitk::DataTreeFilter>::Pointer command = itk::ReceptorMemberCommand<mitk::DataTreeFilter>::New();
  command->SetCallbackFunction(this, &DataTreeFilter::TreePrune);
  m_TreePruneConnection = m_DataTree->AddObserver(itk::TreePruneEvent<mitk::DataTreeBase>(), command);
  }
  
}

DataTreeFilter::~DataTreeFilter()
{
  // remove this as observer from the data tree
  m_DataTree->RemoveObserver( m_TreeNodeChangeConnection );
  m_DataTree->RemoveObserver( m_TreeAddConnection );
  m_DataTree->RemoveObserver( m_TreeRemoveConnection );
  m_DataTree->RemoveObserver( m_TreePruneConnection );
  
  InvokeEvent( mitk::TreeFilterRemoveAllEvent() );
}

void DataTreeFilter::SetPropertiesLabels(const PropertyList labels)
{
  m_PropertyLabels = labels;
  InvokeEvent( mitk::TreeFilterUpdateAllEvent() );
}

const DataTreeFilter::PropertyList& DataTreeFilter::GetPropertiesLabels() const
{
  return m_PropertyLabels;
}

void DataTreeFilter::SetVisibleProperties(const PropertyList keys)
{
  m_VisibleProperties = keys;
  InvokeEvent( mitk::TreeFilterUpdateAllEvent() );
}

const DataTreeFilter::PropertyList& DataTreeFilter::GetVisibleProperties() const
{
  return m_VisibleProperties;
}
      
void DataTreeFilter::SetEditableProperties(const PropertyList keys)
{
  m_EditableProperties = keys;
  InvokeEvent( mitk::TreeFilterUpdateAllEvent() );
}

const DataTreeFilter::PropertyList& DataTreeFilter::GetEditableProperties() const
{
  return m_EditableProperties;
}

void DataTreeFilter::SetRenderer(const mitk::BaseRenderer* renderer)
{
  m_Renderer = renderer;
  InvokeEvent( mitk::TreeFilterUpdateAllEvent() );
}
      
const mitk::BaseRenderer* DataTreeFilter::GetRenderer() const
{
  return m_Renderer;
}

void DataTreeFilter::SetFilter(FilterFunctionPointer filter)
{
  if (m_Filter == filter) return;
  
  if (filter)
    m_Filter = filter;
  else
    m_Filter = &mitk::IsDataTreeNode;

  GenerateModelFromTree();
}
      
const DataTreeFilter::FilterFunctionPointer DataTreeFilter::GetFilter() const
{
  return m_Filter;
}
      
void DataTreeFilter::SetSelectionMode(const SelectionMode selectionMode)
{
  if (m_SelectionMode == selectionMode) return;
  
  m_SelectionMode = selectionMode;
  // TODO update selection (if changed from multi to single)
  // InvokeEvent( mitk::TreeFilterSelectionChanged( item, selected ) );
}

DataTreeFilter::SelectionMode DataTreeFilter::GetSelectionMode() const
{
  return m_SelectionMode;
}
      
void DataTreeFilter::SetHierarchyHandling(const HierarchyHandling hierarchyHandling)
{
  if (m_HierarchyHandling == hierarchyHandling) return;
  
  m_HierarchyHandling = hierarchyHandling;
  
  GenerateModelFromTree();
}

DataTreeFilter::HierarchyHandling DataTreeFilter::GetHierarchyHandling() const
{
  return m_HierarchyHandling;
}

const DataTreeFilter::ItemList* DataTreeFilter::GetItems() const
{
  return m_Items;
}
      
const DataTreeFilter::ItemSet* DataTreeFilter::GetSelectedItems() const
{
  return &m_SelectedItems;
}
      
void DataTreeFilter::SelectItem(const Item* item, bool selected)
{
  // Ich const_caste mir die Welt, wie sie mir gefällt :-)
  // Aber die Items gehören ja dem Filter, deshalb ist das ok
  const_cast<Item*>(item) -> SetSelected(selected);
  InvokeEvent( mitk::TreeFilterSelectionChangedEvent( item, selected ) );
}

void DataTreeFilter::TreeNodeChange(const itk::EventObject& e)
{
  if ( typeid(e) != typeid(itk::TreeNodeChangeEvent<mitk::DataTreeBase>) ) return;
 
  // if event.GetChangePosition()->GetNode() == NULL, then something was removed (while in ~DataTree()) by setting it to NULL
  const itk::TreeChangeEvent<mitk::DataTreeBase>& event( static_cast<const itk::TreeChangeEvent<mitk::DataTreeBase>&>(e) );
  mitk::DataTreeIteratorBase* treePosition = const_cast<mitk::DataTreeIteratorBase*>(&(event.GetChangePosition()));

  if ( treePosition->Get().IsNull() ) return; // TODO this special case is used only in
                                              // ~DataTree(). If used anywhere else, this
                                              // line here has to be removed. But probably
                                              // nobody should set a node to NULL

  GenerateModelFromTree();
}

/// @todo Could TreeAdd be implemented more intelligent?
void DataTreeFilter::TreeAdd(const itk::EventObject& e)
{
  /// Regenerate item tree only from the changed position on (when hierarchy is preserved,
  /// otherwise rebuild everything)
  const itk::TreeAddEvent<mitk::DataTreeBase>& event( static_cast<const itk::TreeAddEvent<mitk::DataTreeBase>&>(e) );
  mitk::DataTreeIteratorBase* treePosition = const_cast<mitk::DataTreeIteratorBase*>(&(event.GetChangePosition()));

  mitk::DataTreeNode* newNode = treePosition->Get();

  if ( !m_Filter(newNode) ) return; // if filter does not match, then nothing has to be done

  /*
     find out, whether there is a item that will be this new nodes parent
     if there is, regenerate the item tree from that parent on
     otherwise, regenerate all items (because then this item will be part of
       the top level items
  */
  
  ItemList* list(m_Items);
  Item* parent(0);

  if ( m_HierarchyHandling == DataTreeFilter::PRESERVE_HIERARCHY && treePosition->HasParent())
    while ( treePosition->HasParent() )
    {
      treePosition->GoToParent();
      if ( m_Filter(treePosition->Get()) )
      {                                         // this is the new parent
        parent = m_Item[treePosition->Get()];
        list = parent->m_Children;
        break; 
      }
    }

  if (parent)
  {
    // regenerate only in part
    InvokeEvent( mitk::TreeFilterRemoveChildrenEvent( parent ) );
    list->clear();
    AddMatchingChildren( treePosition, list, parent, true );
  }
  else
  {
    GenerateModelFromTree();
  }
  
}

void DataTreeFilter::TreePrune(const itk::EventObject& e)
{
  // event has a iterator to the node that is about to be deleted
  const itk::TreeRemoveEvent<mitk::DataTreeBase>& event( static_cast<const itk::TreeRemoveEvent<mitk::DataTreeBase>&>(e) );
  mitk::DataTreeIteratorBase* treePosition = const_cast<mitk::DataTreeIteratorBase*>(&(event.GetChangePosition()));
  
  /*
    if hierarchy is preserved AND if the event's position matches the filter (i.e. an item exists)
        remove that one item from its parent's list
    else      // sub-nodes could be removed that match our filter, even if the removed
              // node does not match
      get preorder iterator from the event's position on
      each iterator position: check for filter matches
        for the first match, 
          get the corresponding item
          mark this item's parent as LIST
          mark the items position in LIST as BEGIN and as END
        for each following match
          increase the position of END
      in LIST: erase all items from BEGIN to END
  */

  Item* item(0);
  ItemList* list(0);
  ItemList::iterator listFirstIter;
  ItemList::iterator listLastIter;
  
  if (   m_HierarchyHandling == DataTreeFilter::PRESERVE_HIERARCHY 
      && m_Filter(treePosition->Get()) )
  {
    item = m_Item[ treePosition->Get() ];

    if ( item->IsRoot() )
      list = m_Items;
    else
      list = item->m_Parent->m_Children;
    
    for ( listFirstIter = list->begin(); listFirstIter != list->end(); ++listFirstIter )
      if ( *listFirstIter == item )
      {
        listLastIter = listFirstIter;
        break;
      }
  }
  else // no hierachy preserved OR removed data tree item not matching the filter
  {
    mitk::DataTreePreOrderIterator treeIter( m_DataTree, treePosition->GetNode() );
    bool firstMatch(true);
    while ( !treeIter.IsAtEnd() )
    {
      if ( m_Filter(treeIter.Get()) )
        if ( firstMatch )
        {
          item = m_Item[ treeIter.Get() ];
          
          if ( item->IsRoot() )
            list = m_Items;
          else
            list = item->m_Parent->m_Children;
  
          listFirstIter = list->begin();
          listLastIter = listFirstIter;

          firstMatch = false;
        }
        else // later matches
        {
          ++listLastIter;
        }
      ++treeIter;    
    }
  }

  // clean selected list
  ++listLastIter; // erase does delete until the item _before_ the second iterator

  InvokeEvent( mitk::TreeFilterRemoveChildrenEvent( item->m_Parent ) );
  list->erase( listFirstIter, listLastIter );
    
  // renumber items of the remaining list
  int i(0);
  for ( listFirstIter = list->begin(); listFirstIter != list->end(); ++listFirstIter, ++i )
  {
    (*listFirstIter)->m_Index = i;
    InvokeEvent( mitk::TreeFilterItemAddedEvent( *listFirstIter ) );
  }

}

void DataTreeFilter::TreeRemove(const itk::EventObject& e)
{
  if ( typeid(e) != typeid(itk::TreeRemoveEvent<mitk::DataTreeBase>) ) return;
  
  // event has a iterator to the node that is about to be deleted
  const itk::TreeRemoveEvent<mitk::DataTreeBase>& event( static_cast<const itk::TreeRemoveEvent<mitk::DataTreeBase>&>(e) );
  mitk::DataTreeIteratorBase* treePosition = const_cast<mitk::DataTreeIteratorBase*>(&(event.GetChangePosition()));
  
  if ( m_Filter(treePosition->Get()) )
  {
    Item* item = m_Item[ treePosition->Get() ];
    ItemList* list(0);
    
    if ( item->IsRoot() )
      list = m_Items;
    else
      list = item->m_Parent->m_Children;
    
    ItemList::iterator listIter;
    for ( listIter = list->begin(); listIter != list->end(); ++listIter )
      if ( *listIter == item ) break;

    if ( item->m_Children->size() > 0 )
    {
      list->insert(listIter, item->m_Children->begin(), item->m_Children->end());
    }
  
    InvokeEvent( mitk::TreeFilterRemoveChildrenEvent( item->m_Parent ) );
    
    // because I'm not sure if the STL guarantees something about the position of an
    // iterator after insert, I once again look for the item to remove

    for ( listIter = list->begin(); listIter != list->end(); ++listIter )
      if ( *listIter == item ) break;
    
    list->erase(listIter);
  
    // renumber items of the remaining list
    int i(0);
    for ( listIter = list->begin(); listIter != list->end(); ++listIter, ++i )
    {
      (*listIter)->m_Index = i;
      InvokeEvent( mitk::TreeFilterItemAddedEvent( *listIter ) );
    }
  }
}

void DataTreeFilter::AddMatchingChildren(mitk::DataTreeIteratorBase* iter, ItemList* list,
    Item* parent, bool verbose )
{
  /* 
  for each child of iter
    check if filter matches
    if it matches:
      create an Item for this child
      iter->GoToChild(thisChild)
      call AddMatchingChildren(iter, newItem.m_ItemList)
      iter->GotoParent();
    if match fails:
      iter->GoToChild(thisChild)
      call AddMatchingChildren(iter, list)  // list from this call
      iter->GotoParent();
  
  Consideration of m_HierarchyHandling adds one branch
  */
  int numChildren( iter->CountChildren() );
  for ( int child = 0; child < numChildren; ++child )
  {
    iter->GoToChild(child);
    if ( m_Filter( iter->Get() ) )
    {
      unsigned int newIndex = list->Size();
      list->CreateElementAt( newIndex ) = Item::New( iter->Get(), this, newIndex, parent );
      if (verbose) InvokeEvent( mitk::TreeFilterItemAddedEvent(m_Items->ElementAt(newIndex)) );

      if ( m_HierarchyHandling == DataTreeFilter::PRESERVE_HIERARCHY )
      {
        AddMatchingChildren( iter, 
                            list->ElementAt(newIndex)->m_Children,
                            list->ElementAt(newIndex).GetPointer(), verbose );
      }
      else
      {
        AddMatchingChildren( iter, list, parent, verbose );
      }
    }
    else
    {
      AddMatchingChildren( iter, list, parent, verbose );
    }
    iter->GoToParent();
  }
}

void DataTreeFilter::GenerateModelFromTree()
{
  InvokeEvent( mitk::TreeFilterRemoveAllEvent() );

  m_Items = ItemList::New(); // clear list (nice thanks to smart pointers)
  mitk::DataTreeIteratorBase* treeIter =  // get an iterator to the data tree
    new mitk::DataTreePreOrderIterator::PreOrderTreeIterator(m_DataTree);

  /*
  if root matches
    create an Item for root
    call AddMatchingChildren(iter, newItem.m_ItemList)
  else (no match on root)
    call AddMatchingChildren(iter, m_ItemList)

  Consideration of m_HierarchyHandling adds one branch
  */
  if (!treeIter->IsAtEnd()) // do nothing if the tree is empty!
  if ( m_Filter( treeIter->Get() ) )
  {
    m_Items->CreateElementAt(0) = Item::New( treeIter->Get(), this, 0, 0 ); // 0 = first item, 0 = no parent
    InvokeEvent( mitk::TreeFilterItemAddedEvent(m_Items->ElementAt(0)) );

    if ( m_HierarchyHandling == DataTreeFilter::PRESERVE_HIERARCHY )
    {
      AddMatchingChildren( treeIter,
                           m_Items->ElementAt(0)->m_Children,
                           m_Items->ElementAt(0).GetPointer(),
                           false );
    }
    else
    {
      AddMatchingChildren( treeIter, m_Items, 0, false );
    }
  }
  else
  {
    AddMatchingChildren( treeIter, m_Items, 0, false );
  }
  
  delete treeIter; // release data tree iterator

  InvokeEvent( mitk::TreeFilterUpdateAllEvent() );
}
      

} // namespace mitk 

// vi: textwidth=90
