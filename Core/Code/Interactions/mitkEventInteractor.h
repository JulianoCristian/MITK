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

#ifndef MITKEVENTHANDLER_H_
#define MITKEVENTHANDLER_H_

#include "itkObject.h"
#include "itkObjectFactory.h"
#include "mitkCommon.h"
#include <MitkExports.h>


namespace mitk
{

  class MITK_CORE_EXPORT EventStateMachine : public EventHandler {

  public:
    mitkClassMacro(EventStateMachine, EventHandler);
    itkNewMacro(Self);

  protected:
    EventStateMachine();
    virtual ~EventStateMachine();


  };

} /* namespace mitk */
#endif /* MITKEVENTHANDLER_H_ */
