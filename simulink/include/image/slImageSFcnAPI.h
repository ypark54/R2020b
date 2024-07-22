/* Copyright 2020 The MathWorks, Inc. */

/**
 * @file: slImageSFcnAPI.h
 *  
 * @brief SimStruct API for Simulink Image operations
 *
 */

#ifndef __SIMSTRUC__IMAGEAPI
#define __SIMSTRUC__IMAGEAPI

#include "simulink_spec.h"
#include "simstruct/simstruc.h"
#include "simstruct/slImageTypes.h"

SIMULINK_EXPORT_EXTERN_C DTypeId ssRegisterImageDataType(SimStruct* S,
                                                         size_t numChannels,
                                                         SSImageColorFormat colorFormat,
                                                         SSImageDataLayout dataLayout,
                                                         BuiltInDTypeId baseType);

SIMULINK_EXPORT_EXTERN_C bool ssIsImageDataType(SimStruct* S, DTypeId dataTypeId);

SIMULINK_EXPORT_EXTERN_C void ssGetInputPortImageSize(SimStruct* S,
                                                      int portIdx,
                                                      size_t* numRows,
                                                      size_t* numColumns,
                                                      size_t* numChannels);
SIMULINK_EXPORT_EXTERN_C void ssGetOutputPortImageSize(SimStruct* S,
                                                       int portIdx,
                                                       size_t* numRows,
                                                       size_t* numColumns,
                                                       size_t* numChannels);

SIMULINK_EXPORT_EXTERN_C void ssSetOutputPortImageSize(SimStruct* S,
                                                       int portIdx,
                                                       size_t numRows,
                                                       size_t numColumns);

SIMULINK_EXPORT_EXTERN_C const void* ssGetInputPortImageData(SimStruct* S, int portIdx);
SIMULINK_EXPORT_EXTERN_C void* ssGetOutputPortImageData(SimStruct* S, int portIdx);

#endif
