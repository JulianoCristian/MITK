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
#ifndef QMITK_STRINGPROPERTYVIEW_H_INCLUDED
#define QMITK_STRINGPROPERTYVIEW_H_INCLUDED

#include "MitkQtWidgetsExtExports.h"
#include <QLabel>
#include <mitkPropertyObserver.h>
#include <mitkStringProperty.h>

/// @ingroup Widgets
class MITKQTWIDGETSEXT_EXPORT QmitkStringPropertyView : public QLabel, public mitk::PropertyView
{
  Q_OBJECT

public:
  QmitkStringPropertyView(const mitk::StringProperty *, QWidget *parent);
  ~QmitkStringPropertyView() override;

protected:
  void PropertyChanged() override;
  void PropertyRemoved() override;

  const mitk::StringProperty *m_StringProperty;
};

#endif
