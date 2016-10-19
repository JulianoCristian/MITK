/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef mitkUIDHelper_h
#define mitkUIDHelper_h

//MITK
#include <mitkDataNode.h>

//MITK
#include "MitkMatchPointRegistrationExports.h"
#include "mitkMatchPointPropertyTags.h"

namespace mitk
{

  typedef std::string NodeUIDType;

  /** Gets the content of the property "matchpoint.uid". If it does not exist, the property will be added with a new UID.
  @pre Passed node is a valid pointer.*/
  NodeUIDType MITKMATCHPOINTREGISTRATION_EXPORT EnsureUID(mitk::DataNode* node);

  /** Helper that checks if the content of property "matchpoint.uid" equals the passed uid. If the property does not exist or node is invalid, return will be false.*/
  bool MITKMATCHPOINTREGISTRATION_EXPORT CheckUID(const mitk::DataNode* node, const NodeUIDType& uid);

}

#endif
