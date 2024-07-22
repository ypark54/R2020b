/* Copyright 1990-2017 The MathWorks, Inc. */

/******************************************************************************
 *                                                                            *
 * File    : RTWCG_FMU2_target.c                                              *
 * Abstract:                                                                  *
 *      Wrapper functions to dynamic call libraries of FMU 2.0                *
 *      This File define functions called by CGIR code                        *
 *      Also handle errors, and logger                                        *
 *                                                                            *
 * Author : Brayden Xia, Nov/2017                                             *
 *                                                                            *
 * TODO:                                                                      *
 *      Support subsystem/ modelref                                           *
 *      Support Tunable Parameters                                            *
 *      Enable FMU Logger                                                     *
 *      Support FMU Model-exchange                                            *
 ******************************************************************************/

#include "RTWCG_FMU2_target.h"
#include <stdlib.h>
#define FMU2_MESSAGE_SIZE 1024

void fmuDebuger(fmi2String message, ...) {
    
    static char debugMsg[FMU2_MESSAGE_SIZE];
    void * diagnostic;
    va_list args;
    va_start (args, message);
    vsnprintf(debugMsg, FMU2_MESSAGE_SIZE, message, args);
    va_end(args);
    
    diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                           CODEGEN_SUPPORT_ARG_STRING_TYPE, debugMsg);
    rt_ssReportDiagnosticAsWarning(NULL, diagnostic);
}

void createParamIdxToOffset(void** fmuv, int array_size){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmustruct->paramIdxToOffset = malloc(sizeof(int) * array_size);
}

void createEnumValueList(void** fmuv, int array_size){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmustruct->enumValueList = malloc(sizeof(int) * array_size);
}

int setParamIdxToOffsetByIdx(void** fmuv, int idx, int value){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    *(fmustruct->paramIdxToOffset + idx) = value;
    return 0;
}

int setEnumValueListByIdx(void** fmuv, int idx, int value){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    *(fmustruct->enumValueList + idx) = value;
    return 0;
}

int getParamIdxToOffsetByIdx(void** fmuv, int idx){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    return *(fmustruct->paramIdxToOffset + idx);
}

int getEnumValueByIdx(void** fmuv, int idx){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    return *(fmustruct->enumValueList + idx);
}
/*
  will do nothing but return a error status
  Whenever a default function is called, it means a functions is called without successful load,
  return a fmi2Error;
*/

static fmi2Status defaultfcn2(fmi2Component c, ...){
    if(c == NULL){ return fmi2Fatal; }
    return fmi2Error;
}
static fmi2Boolean CheckStatus(struct FMU2_CS_RTWCG * fmustruct, fmi2Status status, fmi2String fcnName) {
    
    SimStruct* ss = fmustruct->ssPtr;
    
    if(status == fmi2Error || status == fmi2Fatal){
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimFunctionErrorDebugToDisplayOn", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = status;
        rt_ssSet_slErrMsg(ss, diagnostic);
        ssSetStopRequested(ss, 1);
    }else if(status == fmi2Pending){
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimPendingNotAllowed", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = status;
        rt_ssSet_slErrMsg(ss, diagnostic);
        ssSetStopRequested(ss, 1);
    }
    
    if(status == fmi2OK)
        return fmi2True;
    else
        return fmi2False;
}
/*This function is required by FMI2 standard
  FMU logger is currently not enabled, so it will not be called
  reportasInfo API is currently not avaiable for Rapid accelerator
*/
void fmu2Logger(fmi2ComponentEnvironment c, fmi2String instanceName, fmi2Status status,
                fmi2String category, fmi2String message, ...) {

    (void)  c, instanceName, status, category, message;

    /*va_list args;
    void *diagnostic;
    int prefixLength;
    SimStruct* ssPtr = (SimStruct*) c;

    static const char* strStatus[] = {
        "fmi2OK", "fmi2Warning", "fmi2Discard", "fmi2Error", "fmi2Fatal", "fmi2Pending" };    
    static char translatedMsg[FMU2_MESSAGE_SIZE];
    static char temp[FMU2_MESSAGE_SIZE];
    
    prefixLength = snprintf(translatedMsg, FMU2_MESSAGE_SIZE, "Log from FMU: [category:%s, status:%s] ",
                            strStatus[status], category); 
    va_start (args, message);
    vsnprintf(temp, FMU2_MESSAGE_SIZE, message, args);
    va_end(args);
    
    strncat(translatedMsg, temp, FMU2_MESSAGE_SIZE-prefixLength - 1);

    diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                           CODEGEN_SUPPORT_ARG_STRING_TYPE, translatedMsg);
    rt_ssReportDiagnosticAsWarning(ssPtr, diagnostic);*/
    
}

static _fmi2_default_fcn_type LoadFMUFcn(struct FMU2_CS_RTWCG * fmustruct, const char * fcnName)
{
    _fmi2_default_fcn_type fcn = NULL;
    static char fullFcnName[FULL_FCN_NAME_MAX_LEN];
    memset(fullFcnName, 0, FULL_FCN_NAME_MAX_LEN);
    strncpy(fullFcnName, fmustruct->fmuname, FULL_FCN_NAME_MAX_LEN);
    strncat(fullFcnName, "_", FULL_FCN_NAME_MAX_LEN - strlen(fullFcnName) - 1);
    strncat(fullFcnName, fcnName, FULL_FCN_NAME_MAX_LEN - strlen(fullFcnName) - 1);

#ifdef _WIN32
    fcn = (_fmi2_default_fcn_type)LOAD_FUNCTION(fmustruct->Handle, fcnName);
#else
    *((void **)(&fcn)) = LOAD_FUNCTION(fmustruct->Handle, fcnName);
#endif
    if (fcn == NULL) {
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMULoadLibFunctionError", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = fmi2Warning;
        /*Loading failure will cause a warning, ANY CALL to defualt Fcn will result in an Error and Stop*/
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        fcn = (_fmi2_default_fcn_type) defaultfcn2;
        printf("Fail to load func: %s\n", fcnName);
    }
    return fcn;
}
void* FMU2_fmuInitializeCS( const char * lib, fmi2String instanceName, fmi2String fmuGUID, fmi2String fmuLocation, void* ssPtr, const char * mcclib, const char * mccRes){
    return FMU2_fmuInitialize(lib, instanceName, fmuGUID, fmuLocation, fmi2CoSimulation, ssPtr, mcclib, mccRes);
}
void* FMU2_fmuInitializeME( const char * lib, fmi2String instanceName, fmi2String fmuGUID, fmi2String fmuLocation, void* ssPtr, const char * mcclib, const char * mccRes){
    return FMU2_fmuInitialize(lib, instanceName, fmuGUID, fmuLocation, fmi2ModelExchange, ssPtr, mcclib, mccRes);
}

/*This is to mute MS complier for reporting warning C4232*/
static void* local_calloc(size_t num, size_t aSize)
{
    return calloc(num, aSize);
}
static void local_free(void* aPtr)
{
    free(aPtr);
}

/* fmuPrefix is instanceName */
void* FMU2_fmuInitialize( const char * lib, fmi2String instanceName, fmi2String fmuGUID, fmi2String fmuLocation, fmi2Type fmuType, void* ssPtr, const char * mcclib, const char * mccRes) {

    struct FMU2_CS_RTWCG * fmustruct;
    fmi2Boolean visible, isLoggingOn;
    void * diagnostic;
    static fmi2CallbackFunctions fmi2callbacks = {fmu2Logger, local_calloc, local_free, NULL, NULL}; /*TODO: NULL last may affect logging*/
    
    fmustruct = (struct FMU2_CS_RTWCG *)calloc(1, sizeof(struct FMU2_CS_RTWCG));
    fmustruct->ssPtr = (SimStruct*) ssPtr;
    fmustruct->fmuname = (char *)instanceName;
    fmustruct->dllfile = (char *)lib;
    if(strlen(mcclib) != 0)
        fmustruct->dllfile = (char *)mcclib;
    fmustruct->FMUErrorStatus = fmi2OK;
    
    if (strlen(instanceName)+ FCN_NAME_MAX_LEN + 1 >= FULL_FCN_NAME_MAX_LEN){
        /*FMU name is longer than 200+, rarely happens*/
        diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                               CODEGEN_SUPPORT_ARG_STRING_TYPE, "FMU Name is too long.");
        fmustruct->FMUErrorStatus = fmi2Fatal;
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        ssSetStopRequested(fmustruct->ssPtr, 1);
        return NULL;
    }
    fmustruct->Handle = LOAD_LIBRARY(fmustruct->dllfile);
    if (NULL == fmustruct->Handle) {
        diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMULoadLibraryError", 2,
                                               CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->dllfile,
                                               CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        /*loading lib failure will cause an Fatal and Stop*/
        fmustruct->FMUErrorStatus = fmi2Fatal;
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        ssSetStopRequested(fmustruct->ssPtr, 1);
        CLOSE_LIBRARY(fmustruct->Handle);
        return NULL;
    }
    
    /*instantiate parameters*/
    visible = fmi2False;   /* no simulator user interface*/
    isLoggingOn = fmi2False;  /*logging is currently disabled, on for debug*/    

    /* load fmi functions */
    LoadFMU2CommonFunctions(fmustruct);
    if(fmuType == fmi2CoSimulation){
        LoadFMU2CSFunctions(fmustruct);
    }else if(fmuType == fmi2ModelExchange){
        LoadFMU2MEFunctions(fmustruct);
    }

    char* fmuResLocation = (char*) fmuLocation;
    if(strlen(mccRes) != 0)
        fmuResLocation = (char*) mccRes;
    /* instantiate fmu */
    fmustruct->mFMIComp = fmustruct->instantiate(instanceName,
                                                 fmuType,
                                                 fmuGUID,
                                                 fmuResLocation,
                                                 &fmi2callbacks,
                                                 visible,
                                                 isLoggingOn);  
    if (NULL == fmustruct->mFMIComp ){
        CheckStatus(fmustruct, fmi2Error, "fmi2Instantiate");
        return NULL;
    }
    return (void *) fmustruct;
}

fmi2Status FMU2_setupExperiment(void** fmuv, fmi2Boolean isToleranceUsed, fmi2Real toleranceValue, fmi2Real currentTime, fmi2Boolean isTFinalUsed, fmi2Real TFinal){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->setupExperiment(fmustruct->mFMIComp, isToleranceUsed, toleranceValue, currentTime, isTFinalUsed, TFinal);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetupExperiment");
}

fmi2Boolean FMU2_enterInitializationMode(void** fmuv){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->enterInitializationMode(fmustruct->mFMIComp);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2EnterInitializationMode");
}
fmi2Boolean FMU2_exitInitializationMode(void** fmuv){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->exitInitializationMode(fmustruct->mFMIComp);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2ExitInitializationMode");
}

fmi2Boolean FMU2_terminate(void **fmuv){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    if(fmustruct->FMUErrorStatus != fmi2Fatal){
        fmi2Status fmi2Flag = fmustruct->terminate(fmustruct->mFMIComp);
        CheckStatus(fmustruct, fmi2Flag, "fmi2TerminateSlave");
        if(fmustruct->FMUErrorStatus != fmi2Error)
            fmustruct->freeInstance(fmustruct->mFMIComp);
    }
    CLOSE_LIBRARY(fmustruct->Handle);
    free(fmustruct->paramIdxToOffset);
    free(fmustruct->enumValueList);
    free(fmustruct);
    return fmi2True;
}

fmi2Boolean FMU2_freeInstance(void **fmuv){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmustruct->freeInstance(fmustruct->mFMIComp);
    return fmi2True;
}


fmi2Status FMU2_doStep(void **fmuv, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint){
    static char time[10];
    void * diagnostic;
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->doStep(fmustruct->mFMIComp, currentCommunicationPoint,communicationStepSize, noSetFMUStatePriorToCurrentPoint);
    if(fmi2Flag == fmi2Discard){
         fmi2Boolean boolVal;
         fmustruct->getBooleanStatus(fmustruct->mFMIComp, fmi2Terminated, &boolVal);
         if(boolVal == fmi2True){
             snprintf(time, 10, "%f", currentCommunicationPoint);
             diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU2:FMU2SimDoStepTerminated", 2,
                                                    CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname,
                                                    CODEGEN_SUPPORT_ARG_STRING_TYPE, time);
             /*loading lib failure will cause an Fatal and Stop*/
             rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
             ssSetStopRequested(fmustruct->ssPtr, 1);
         }
    }
    return CheckStatus(fmustruct, fmi2Flag, "fmi2DoStep");
}
fmi2Status FMU2_setRealVal(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2Real dvalue){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Real value = dvalue;
    fmi2Status fmi2Flag = fmustruct->setReal(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetReal");
}

fmi2Status FMU2_setReal(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2Real value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->setReal(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetReal");
}

fmi2Status FMU2_getReal(void **fmuv, const fmi2ValueReference dvr, size_t nvr, fmi2Real value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->getReal(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetReal");
}

fmi2Status FMU2_setIntegerVal(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2Integer dvalue){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Integer value = dvalue;
    fmi2Status fmi2Flag = fmustruct->setInteger(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetInteger");
}

fmi2Status FMU2_setInteger(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2Integer value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->setInteger(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetInteger");

}

fmi2Status FMU2_getInteger(void **fmuv, const fmi2ValueReference dvr, size_t nvr, fmi2Integer value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->getInteger(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetInteger");
}

fmi2Status FMU2_setBooleanVal(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2Boolean dvalue){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Boolean value = dvalue;
    fmi2Status fmi2Flag = fmustruct->setBoolean(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetBoolean");
}

fmi2Status FMU2_setBoolean(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2Boolean value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->setBoolean(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetBoolean");
}

fmi2Status FMU2_getBoolean(void **fmuv, const fmi2ValueReference dvr, size_t nvr, fmi2Boolean value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->getBoolean(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetBoolean");
}

fmi2Status FMU2_setStringVal(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2String dvalue){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2String value = dvalue;
    fmi2Status fmi2Flag = fmustruct->setString(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmi2Flag, "fmiSetString");
}

fmi2Status FMU2_setString(void **fmuv, const fmi2ValueReference dvr, size_t nvr, const fmi2String value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->setString(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetString");
}

fmi2Status FMU2_getString(void **fmuv, const fmi2ValueReference dvr, size_t nvr, fmi2String value[]){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2ValueReference vr =dvr;
    fmi2Status fmi2Flag = fmustruct->getString(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetString");
}

/*Load FMU 2 Common Functions*/
void LoadFMU2CommonFunctions(struct FMU2_CS_RTWCG* fmustruct){
    fmustruct->getTypesPlatform   = (fmi2GetTypesPlatformTYPE*)      LoadFMUFcn(fmustruct, "fmi2GetTypesPlatform");
    fmustruct->getVersion         = (fmi2GetVersionTYPE*)            LoadFMUFcn(fmustruct, "fmi2GetVersion");
    fmustruct->setDebugLogging    = (fmi2SetDebugLoggingTYPE*)       LoadFMUFcn(fmustruct, "fmi2SetDebugLogging");
    fmustruct->instantiate        = (fmi2InstantiateTYPE*)           LoadFMUFcn(fmustruct, "fmi2Instantiate");
    fmustruct->freeInstance       = (fmi2FreeInstanceTYPE*)          LoadFMUFcn(fmustruct, "fmi2FreeInstance");
    fmustruct->setupExperiment    = (fmi2SetupExperimentTYPE*)       LoadFMUFcn(fmustruct, "fmi2SetupExperiment");
    fmustruct->enterInitializationMode  = (fmi2EnterInitializationModeTYPE*)    LoadFMUFcn(fmustruct, "fmi2EnterInitializationMode");
    fmustruct->exitInitializationMode   = (fmi2ExitInitializationModeTYPE*)     LoadFMUFcn(fmustruct, "fmi2ExitInitializationMode");
    fmustruct->terminate          = (fmi2TerminateTYPE*)             LoadFMUFcn(fmustruct, "fmi2Terminate");
    fmustruct->reset              = (fmi2ResetTYPE*)                 LoadFMUFcn(fmustruct, "fmi2Reset");
    
    fmustruct->setReal            = (fmi2SetRealTYPE*)               LoadFMUFcn(fmustruct, "fmi2SetReal");
    fmustruct->setInteger         = (fmi2SetIntegerTYPE*)            LoadFMUFcn(fmustruct, "fmi2SetInteger");
    fmustruct->setBoolean         = (fmi2SetBooleanTYPE*)            LoadFMUFcn(fmustruct, "fmi2SetBoolean");
    fmustruct->setString          = (fmi2SetStringTYPE*)             LoadFMUFcn(fmustruct, "fmi2SetString");
    fmustruct->getReal            = (fmi2GetRealTYPE*)               LoadFMUFcn(fmustruct, "fmi2GetReal");
    fmustruct->getInteger         = (fmi2GetIntegerTYPE*)            LoadFMUFcn(fmustruct, "fmi2GetInteger");
    fmustruct->getBoolean         = (fmi2GetBooleanTYPE*)            LoadFMUFcn(fmustruct, "fmi2GetBoolean");
    fmustruct->getString          = (fmi2GetStringTYPE*)             LoadFMUFcn(fmustruct, "fmi2GetString");

    fmustruct->getFMUstate        = (fmi2GetFMUstateTYPE*)           LoadFMUFcn(fmustruct, "fmi2GetFMUstate");
    fmustruct->setFMUstate        = (fmi2SetFMUstateTYPE*)           LoadFMUFcn(fmustruct, "fmi2SetFMUstate");
    fmustruct->freeFMUstate       = (fmi2FreeFMUstateTYPE*)          LoadFMUFcn(fmustruct, "fmi2FreeFMUstate");
    fmustruct->serializedFMUstateSize    = (fmi2SerializedFMUstateSizeTYPE*)    LoadFMUFcn(fmustruct, "fmi2SerializedFMUstateSize");
    fmustruct->serializeFMUstate         = (fmi2SerializeFMUstateTYPE*)          LoadFMUFcn(fmustruct, "fmi2SerializeFMUstate");
    fmustruct->deSerializeFMUstate       = (fmi2DeSerializeFMUstateTYPE*)        LoadFMUFcn(fmustruct, "fmi2DeSerializeFMUstate");         
    fmustruct->getDirectionalDerivative  = (fmi2GetDirectionalDerivativeTYPE*)   LoadFMUFcn(fmustruct, "fmi2GetDirectionalDerivative");
}
/*Load FMU 2 CS Common Functions*/
void LoadFMU2CSFunctions(struct FMU2_CS_RTWCG* fmustruct){
    fmustruct->setRealInputDerivatives = (fmi2SetRealInputDerivativesTYPE*)   LoadFMUFcn(fmustruct, "fmi2SetRealInputDerivatives");
    fmustruct->getRealOutputDerivatives = (fmi2GetRealOutputDerivativesTYPE*) LoadFMUFcn(fmustruct, "fmi2GetRealOutputDerivatives");
    fmustruct->cancelStep          = (fmi2CancelStepTYPE*)            LoadFMUFcn(fmustruct, "fmi2CancelStep");
    fmustruct->doStep              = (fmi2DoStepTYPE*)                LoadFMUFcn(fmustruct, "fmi2DoStep");
    fmustruct->getStatus           = (fmi2GetStatusTYPE*)             LoadFMUFcn(fmustruct, "fmi2GetStatus");
    fmustruct->getRealStatus       = (fmi2GetRealStatusTYPE*)         LoadFMUFcn(fmustruct, "fmi2GetRealStatus");
    fmustruct->getIntegerStatus    = (fmi2GetIntegerStatusTYPE*)      LoadFMUFcn(fmustruct, "fmi2GetIntegerStatus");
    fmustruct->getBooleanStatus    = (fmi2GetBooleanStatusTYPE*)      LoadFMUFcn(fmustruct, "fmi2GetBooleanStatus");
    fmustruct->getStringStatus     = (fmi2GetStringStatusTYPE*)       LoadFMUFcn(fmustruct, "fmi2GetStringStatus");
}

/*Load FMU 2 ME Common Functions*/
void LoadFMU2MEFunctions(struct FMU2_CS_RTWCG* fmustruct){
    /*fmi me functions*/
    fmustruct->enterEventMode    = (fmi2EnterEventModeTYPE*) LoadFMUFcn(fmustruct, "fmi2EnterEventMode");
    fmustruct->newDiscreteStates = (fmi2NewDiscreteStatesTYPE*) LoadFMUFcn(fmustruct, "fmi2NewDiscreteStates");
    fmustruct->enterContinuousTimeMode = (fmi2EnterContinuousTimeModeTYPE*) LoadFMUFcn(fmustruct, "fmi2EnterContinuousTimeMode");
    fmustruct->completedIntegratorStep = (fmi2CompletedIntegratorStepTYPE*) LoadFMUFcn(fmustruct, "fmi2CompletedIntegratorStep");
    fmustruct->setTime             = (fmi2SetTimeTYPE*) LoadFMUFcn(fmustruct, "fmi2SetTime");
    fmustruct->setContinuousStates = (fmi2SetContinuousStatesTYPE*) LoadFMUFcn(fmustruct,"fmi2SetContinuousStates");
    fmustruct->getDerivatives      = (fmi2GetDerivativesTYPE*) LoadFMUFcn(fmustruct, "fmi2GetDerivatives");
    fmustruct->getEventIndicators  = (fmi2GetEventIndicatorsTYPE*) LoadFMUFcn(fmustruct, "fmi2GetEventIndicators");
    fmustruct->getContinuousStates = (fmi2GetContinuousStatesTYPE*) LoadFMUFcn(fmustruct,"fmi2GetContinuousStates");
    fmustruct->getNominalsOfContinuousStates = (fmi2GetNominalsOfContinuousStatesTYPE*) LoadFMUFcn(fmustruct, "fmi2GetNominalsOfContinuousStates");
}


/*me standard functions wrapper*/
fmi2Status FMU2_enterEventMode(void** fmuv) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->enterEventMode(fmustruct->mFMIComp);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2EnterEventMode");
}

/* this function was directly called in eventIteration, no wrapper needed*/
/* fmi2Status FMU2_newDiscreteStates(void** fmuv, fmi2EventInfo* eventInfo); */

fmi2Status FMU2_enterContinuousTimeMode(void** fmuv) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->enterContinuousTimeMode(fmustruct->mFMIComp);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2EnterContinuousTimeMode");
}

fmi2Status FMU2_completedIntegratorStep(void** fmuv, fmi2Boolean noSetFMUStatePriorToCurrentPoint, fmi2Boolean* enterEventMode, fmi2Boolean* terminateSimulation) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->completedIntegratorStep(fmustruct->mFMIComp, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2CompletedIntegratorStep");

}
/* Providing independent variables and re-initialization of caching */
fmi2Status FMU2_setTime(void** fmuv, fmi2Real time) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->setTime(fmustruct->mFMIComp, time);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetTime");
}

fmi2Status FMU2_setContinuousStates (void** fmuv, const fmi2Real states[], size_t nx)  {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->setContinuousStates(fmustruct->mFMIComp, states, nx);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2SetContinuousStates");
}

/* Evaluation of the model equations */
fmi2Status FMU2_getDerivatives(void** fmuv, fmi2Real derivatives[], size_t nx) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->getDerivatives(fmustruct->mFMIComp, derivatives, nx);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetDerivatives");
}

fmi2Status FMU2_getEventIndicators(void** fmuv, fmi2Real eventIndicators[], size_t nx) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->getEventIndicators(fmustruct->mFMIComp, eventIndicators, nx);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetEventIndicators");
}

fmi2Status FMU2_getContinuousStates (void** fmuv, fmi2Real states[], size_t nx)  {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->getContinuousStates(fmustruct->mFMIComp, states, nx);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetContinuousStates");
}

fmi2Status FMU2_getNominalsOfContinuousStates (void** fmuv, fmi2Real states[], size_t nx) {
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmustruct->getNominalsOfContinuousStates(fmustruct->mFMIComp, states, nx);
    return CheckStatus(fmustruct, fmi2Flag, "fmi2GetNominalsOfContinuousStates");
}

/* me helper functions*/
void FMU2_getNextEventTime(void **fmuv, fmi2Real* nextEventTime, int32_T* upcomingTimeEvent){
    struct FMU2_CS_RTWCG * fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    *nextEventTime = fmustruct->eventInfo.nextEventTime;
    *upcomingTimeEvent = (int32_T) fmustruct->eventInfo.nextEventTimeDefined;
}

void FMU2_simTerminate(void **fmuv, const char* blkPath, fmi2Real time){
    struct FMU2_CS_RTWCG* fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    /* terminate the simulation (successfully) */
    /*void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU2:FMU2SimEventUpdateTerminated", 2,
                                                  CODEGEN_SUPPORT_ARG_STRING_TYPE, blkPath,
                                                  CODEGEN_SUPPORT_ARG_REAL_TYPE, time);
    rt_ssReportDiagnosticAsInfo(fmustruct->ssPtr, diagnostic);*/  /*todo: wait for report info API available in accel SimTarget*/
    ssSetStopRequested(fmustruct->ssPtr, 1);
}

void FMU2_eventIteration(void **fmuv,  const char* blkPath, fmi2Real time){
    struct FMU2_CS_RTWCG* fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    fmi2Status fmi2Flag = fmi2OK;
    int iterationNumber = 0;
    
    fmustruct->eventInfo.newDiscreteStatesNeeded = fmi2True;
    while(fmustruct->eventInfo.newDiscreteStatesNeeded == fmi2True){
        /*safe call to newDiscreteStates*/
        fmi2Flag = fmustruct->newDiscreteStates(fmustruct->mFMIComp, &(fmustruct->eventInfo));
        CheckStatus(fmustruct, fmi2Flag, "fmi2NewDiscreteStates");

        if(fmustruct->eventInfo.terminateSimulation == fmi2True){
            /* terminate the simulation (successfully) */
            FMU2_simTerminate(fmuv, blkPath, time);
        }

        if(iterationNumber >= 10000){
            void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimEventUpdateTerminated", 2,
                                                          CODEGEN_SUPPORT_ARG_REAL_TYPE, time,
                                                          CODEGEN_SUPPORT_ARG_INTEGER_TYPE, iterationNumber);
            rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
            break;
            
        } else
            iterationNumber ++;
    }
}

fmi2Boolean FMU2_valuesOfContinuousStatesChanged(void **fmuv){
    struct FMU2_CS_RTWCG* fmustruct = (struct FMU2_CS_RTWCG *)(*fmuv);
    return fmustruct->eventInfo.valuesOfContinuousStatesChanged;
}

