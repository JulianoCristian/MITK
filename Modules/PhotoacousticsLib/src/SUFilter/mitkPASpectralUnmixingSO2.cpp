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

#include "mitkPASpectralUnmixingSO2.h"

// ImageAccessor
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>

mitk::pa::SpectralUnmixingSO2::SpectralUnmixingSO2()
{
  this->SetNumberOfIndexedOutputs(1);
  this->SetNthOutput(0, mitk::Image::New());
}

mitk::pa::SpectralUnmixingSO2::~SpectralUnmixingSO2()
{

}

void mitk::pa::SpectralUnmixingSO2::GenerateData()
{
  MITK_INFO << "GENERATING DATA..";

  // Get input image
  mitk::Image::Pointer inputHbO2 = GetInput(0);
  mitk::Image::Pointer inputHb = GetInput(1);


  unsigned int xDim = inputHbO2->GetDimensions()[0];
  unsigned int yDim = inputHbO2->GetDimensions()[1];
  unsigned int size = xDim * yDim;

  MITK_INFO << "x dimension: " << xDim;
  MITK_INFO << "y dimension: " << yDim;

  InitializeOutputs();

  // Copy input image into array
  mitk::ImageReadAccessor readAccessHbO2(inputHbO2);
  mitk::ImageReadAccessor readAccessHb(inputHb);

  const float* inputDataArrayHbO2 = ((const float*)readAccessHbO2.GetData());
  const float* inputDataArrayHb = ((const float*)readAccessHb.GetData());

  CheckPreConditions(inputHbO2, inputHb);

  //loop over every pixel @ x,y plane
  for (unsigned int x = 0; x < xDim; x++)
  {
    for (unsigned int y = 0; y < yDim; y++)
    {
      unsigned int pixelNumber = x * yDim + y;
      float pixelHb = inputDataArrayHb[pixelNumber];
      float pixelHbO2 = inputDataArrayHbO2[pixelNumber];
      float resultSO2 = calculateSO2(pixelHb, pixelHbO2);
      auto output = GetOutput(0);
      mitk::ImageWriteAccessor writeOutput(output);
      float* writeBuffer = (float *)writeOutput.GetData();
      writeBuffer[x*yDim + y] = resultSO2;
    }
  }
  MITK_INFO << "GENERATING DATA...[DONE]";
}

void mitk::pa::SpectralUnmixingSO2::CheckPreConditions(mitk::Image::Pointer inputHbO2, mitk::Image::Pointer inputHb)
{
  unsigned int xDimHb = inputHb->GetDimensions()[0];
  unsigned int yDimHb = inputHb->GetDimensions()[1];
  unsigned int sizeHb = xDimHb * yDimHb;

  unsigned int xDimHbO2 = inputHbO2->GetDimensions()[0];
  unsigned int yDimHbO2 = inputHbO2->GetDimensions()[1];
  unsigned int sizeHbO2 = xDimHbO2 * yDimHbO2;

  if (sizeHb != sizeHbO2)
    mitkThrow() << "SIZE ERROR!";

  if (xDimHb != xDimHbO2 || yDimHb != yDimHbO2)
    mitkThrow() << "DIMENTIONALITY ERROR!";

  MITK_INFO << "CHECK PRECONDITIONS ...[DONE]";
}

void mitk::pa::SpectralUnmixingSO2::InitializeOutputs()
{
  unsigned int numberOfInputs = GetNumberOfIndexedInputs();
  unsigned int numberOfOutputs = GetNumberOfIndexedOutputs();
  MITK_INFO << "InputsSO2: " << numberOfInputs << " OutputsSO: " << numberOfOutputs;

  //  Set dimensions (2) and pixel type (float) for output
  mitk::PixelType pixelType = mitk::MakeScalarPixelType<float>();
  const int NUMBER_OF_SPATIAL_DIMENSIONS = 2;
  auto* dimensions = new unsigned int[NUMBER_OF_SPATIAL_DIMENSIONS];
  for(unsigned int dimIdx=0; dimIdx<NUMBER_OF_SPATIAL_DIMENSIONS; dimIdx++)
  {
    dimensions[dimIdx] = GetInput()->GetDimensions()[dimIdx];
  }

  // Initialize numberOfOutput pictures with pixel type and dimensions
  for (unsigned int outputIdx = 0; outputIdx < numberOfOutputs; outputIdx++)
  {
    GetOutput(outputIdx)->Initialize(pixelType, NUMBER_OF_SPATIAL_DIMENSIONS, dimensions);
  }
}

float mitk::pa::SpectralUnmixingSO2::calculateSO2(float Hb, float HbO2)
{
  float result = HbO2 / (Hb + HbO2);
  return result;
}
