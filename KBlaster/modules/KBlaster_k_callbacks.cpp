#include "KBlaster_k_callbacks.hpp"








/*
* ------------------------------------------------------------------------------------------------------------------------------------
* KBlast_GetProcessNotifyCallbackArray gets a pointer to an array which contains
* kernel handles to callbacks of type Process Creation
* ------------------------------------------------------------------------------------------------------------------------------------
*/
PVOID KBlast_GetCallbackStoragePointer(IN CALLBACK_TYPE cType)
{
	ULONG64 pInitialFunction = 0;
	ULONG64 pInnerFunction = 0;
	ULONG64 pStorage = 0;
	ULONG64 i, offset = 0;
	UCHAR initialOpcode1 = 0xE8, initialOpcode2 = 0xE9;
	UCHAR innerOpcode1 = 0, innerOpcode2 = 0;
	UNICODE_STRING initialFunctionName = { 0 };

	switch (cType)
	{
	case ARRAY_PROCESS:
		RtlInitUnicodeString(&initialFunctionName, L"PsSetCreateProcessNotifyRoutine");
		pInitialFunction = (ULONG64)MmGetSystemRoutineAddress(&initialFunctionName);
		innerOpcode1 = 0x4C;
		innerOpcode2 = 0x8D;
		break;

	case ARRAY_THREAD:
		RtlInitUnicodeString(&initialFunctionName, L"PsSetCreateThreadNotifyRoutine");
		pInitialFunction = (ULONG64)MmGetSystemRoutineAddress(&initialFunctionName);
		innerOpcode1 = 0x48;
		innerOpcode2 = 0x8D;
		break;

	case ARRAY_IMAGE:
		RtlInitUnicodeString(&initialFunctionName, L"PsSetLoadImageNotifyRoutine");
		pInitialFunction = (ULONG64)MmGetSystemRoutineAddress(&initialFunctionName);
		innerOpcode1 = 0x48;
		innerOpcode2 = 0x8D;
		break;

	case LISTENTRY_REGISTRY:
		RtlInitUnicodeString(&initialFunctionName, L"CmUnRegisterCallback");
		pInitialFunction = (ULONG64)MmGetSystemRoutineAddress(&initialFunctionName);
		initialOpcode1 = 0x48;
		initialOpcode2 = 0x8D;
		break;

	case LISTENTRY_OBJECT:
		DbgPrint("Not implemented yet.\n");
		break;

	default:
		break;
	}

	if ((cType == ARRAY_PROCESS) || (cType == ARRAY_THREAD) || (cType == ARRAY_IMAGE))
	{
		if (pInitialFunction != 0)
		{
			for (i = pInitialFunction; i < pInitialFunction + 0x70; i++)
			{
				if ((*(PUCHAR)i == initialOpcode1) || (*(PUCHAR)i == initialOpcode2))
				{
					RtlCopyMemory(&offset, (PUCHAR)i + 1, 4);
					pInnerFunction = i + offset + 5;

					for (i = pInnerFunction; i < pInnerFunction + 0x70; i++)
					{
						if ((*(PUCHAR)i == innerOpcode1) && (*((PUCHAR)i + 1) == innerOpcode2))
						{
							offset = 0;
							RtlCopyMemory(&offset, (PUCHAR)i + 3, 4);
							pStorage = i + offset + 7;
							break;
						}
					}
				}
			}
		}
	}

	if ((cType == LISTENTRY_REGISTRY) || (cType == LISTENTRY_OBJECT))
	{
		if (pInitialFunction != 0)
		{
			for (i = pInitialFunction; i < (pInitialFunction + 150); i++)
			{
				if ((*(PUCHAR)i == initialOpcode1) && (*((PUCHAR)i + 1) == initialOpcode2)) // if lea rcx, [nt!CallbackListHead]
				{
					RtlCopyMemory(&offset, (PUCHAR)i + 3, 4);
					pStorage = i + offset + 7;
				}
			}
		}
	}

	

	return (PVOID)pStorage;

}


/*
* ------------------------------------------------------------------------------------------------------------------------------------
* Given a number of modules X, KBlast_GetKernelCallbackModuleNumber gets the module number
* a specified callback belongs to. It returns 0xffffffff if the callback address does not
* exist in any driver image space.
* ------------------------------------------------------------------------------------------------------------------------------------
*/
ULONG KBlast_GetKernelCallbackModuleNumber(IN PVOID callback, IN ULONG nModules, IN PAUX_MODULE_EXTENDED_INFO pAuxModuleExtendedInfo)
{
	ULONG64 arithmCallback = 0;
	ULONG64 ImageBaseAddress = 0, ImageEndAddress = 0;
	ULONG moduleNumber = 0;
	arithmCallback = (ULONG64)callback;

	for (moduleNumber = 0; moduleNumber < nModules; moduleNumber++)
	{
		ImageBaseAddress = (ULONG64)pAuxModuleExtendedInfo[moduleNumber].BasicInfo.ImageBase;
		ImageEndAddress = (ULONG64)(ImageBaseAddress + pAuxModuleExtendedInfo[moduleNumber].ImageSize);

		if ((arithmCallback < ImageEndAddress) && (arithmCallback > ImageBaseAddress))
		{
			return moduleNumber;
		}
	}

	return 0xffffffff;

}




NTSTATUS KBlast_GetCallbackListEntryInformation(IN PVOID pListEntryHead, OUT OPTIONAL PPROCESS_KERNEL_CALLBACK_ARRAY pOutBuf)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PCMREG_CALLBACK cmRegCallbackStruct = 0;
	PAUX_MODULE_EXTENDED_INFO pAuxModuleExtendedInfo = 0;
	PVOID ret = 0;
	ULONG bufferSize = 0, nModules = 0, moduleNumber = 0, structCounter = 0;
	ULONG64 Callback = 0, newStart = 0;
	int CallbackQuota = 0, zeroHandles = 0;

	
	cmRegCallbackStruct = (PCMREG_CALLBACK)pListEntryHead;
	status = AuxKlibQueryModuleInformation(&bufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);
	if ((status == STATUS_SUCCESS) && (bufferSize != 0))
	{
		pAuxModuleExtendedInfo = (PAUX_MODULE_EXTENDED_INFO)ExAllocatePoolWithTag(PagedPool, bufferSize, POOL_TAG);
		if ((PVOID)pAuxModuleExtendedInfo != 0)
		{
			RtlZeroMemory(pAuxModuleExtendedInfo, bufferSize);
			status = AuxKlibQueryModuleInformation(&bufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), pAuxModuleExtendedInfo);
			if (NT_SUCCESS(status))
			{
				nModules = bufferSize / sizeof(AUX_MODULE_EXTENDED_INFO); // number of modules
				
				pOutBuf->Array = pListEntryHead;
				newStart = (ULONG64)pListEntryHead;
				while (ret != pListEntryHead)
				{
					if ((ULONG64)newStart == (ULONG64)pListEntryHead)
					{
						Callback = *(PULONG64)((DWORD_PTR)newStart + FIELD_OFFSET(CMREG_CALLBACK, Function) + sizeof(PVOID));
					}
					else
					{
						Callback = *(PULONG64)((DWORD_PTR)newStart + FIELD_OFFSET(CMREG_CALLBACK, Function));
					}

					if (Callback != 0)
					{
						CallbackQuota++;
						moduleNumber = KBlast_GetKernelCallbackModuleNumber((PVOID)Callback, nModules, pAuxModuleExtendedInfo);
						if (moduleNumber != 0xffffffff)
						{
							pOutBuf->CallbackInformation[structCounter].CallbackFunctionPointer = (PVOID)Callback;
							pOutBuf->CallbackInformation[structCounter].ModuleInformation.ModuleBase = pAuxModuleExtendedInfo[moduleNumber].BasicInfo.ImageBase;
							pOutBuf->CallbackInformation[structCounter].ModuleInformation.ModuleFileNameOffset = pAuxModuleExtendedInfo[moduleNumber].FileNameOffset;
							pOutBuf->CallbackInformation[structCounter].ModuleInformation.ModuleImageSize = pAuxModuleExtendedInfo[moduleNumber].ImageSize;
							if (strlen((const char*)pAuxModuleExtendedInfo[moduleNumber].FullPathName) < AUX_KLIB_MODULE_PATH_LEN)
							{
								strcpy(pOutBuf->CallbackInformation[structCounter].ModuleInformation.ModuleFullPathName, (const char*)pAuxModuleExtendedInfo[moduleNumber].FullPathName);
							}
							structCounter++;
						}
					}
					else
					{
						zeroHandles++;
					}

					newStart = *(PULONG64)(newStart + sizeof(PVOID));
					ret = (PVOID)newStart;

				}

				pOutBuf->CallbackQuota = CallbackQuota - zeroHandles;

			}

			goto cleanup;

		}
	}

cleanup:

	if (pAuxModuleExtendedInfo != 0)
	{
		ExFreePoolWithTag(pAuxModuleExtendedInfo, POOL_TAG);
	}
	if (!NT_SUCCESS(status))
	{
		pOutBuf->Array = 0;
	}

	structCounter = 0;

	return status;

}





/*
* ------------------------------------------------------------------------------------------------------------------------------------
* KBlast_GetProcessKernelCallbackArrayInformation retrieves extensive information about the entire PspSetCreateProcessNotifyRoutine,
* including information about the single callback routines. This information gets stored inside a PROCESS_KERNEL_CALLBACK_ARRAY
* struct.
* ------------------------------------------------------------------------------------------------------------------------------------
*/
NTSTATUS KBlast_GetCallbackArrayInformation(IN PVOID ProcessNotifyCallbackArray, OUT PULONG szNeeded, OUT OPTIONAL PPROCESS_KERNEL_CALLBACK_ARRAY pProcessKernelCallbackArray)
{
	NTSTATUS status = 0;
	ULONG bufferSize = 0, nModules = 0, moduleNumber = 0, structCounter = 0;
	ULONG64 callbackHandle = 0, Callback = 0;
	PAUX_MODULE_EXTENDED_INFO pAuxModuleExtendedInfo = 0;
	int CallbackQuota = 0, zeroHandles = 0, i = 0, maxCallbacks = 44;


	status = AuxKlibQueryModuleInformation(&bufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);
	if ((status == STATUS_SUCCESS) && (bufferSize != 0))
	{
		pAuxModuleExtendedInfo = (PAUX_MODULE_EXTENDED_INFO)ExAllocatePoolWithTag(PagedPool, bufferSize, POOL_TAG);
		if ((PVOID)pAuxModuleExtendedInfo != 0)
		{
			RtlZeroMemory(pAuxModuleExtendedInfo, bufferSize);
			status = AuxKlibQueryModuleInformation(&bufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), pAuxModuleExtendedInfo);
			if (NT_SUCCESS(status))
			{
				nModules = bufferSize / sizeof(AUX_MODULE_EXTENDED_INFO);

				while (i < (8 * maxCallbacks)) // enumerate up to 44 callbacks
				{
					callbackHandle = (ULONG64)((*(ULONG64*)(ULONG64*)((ULONG_PTR)ProcessNotifyCallbackArray + i)));
					if (callbackHandle == 0)
					{
						zeroHandles++;
					}

					CallbackQuota++;
					i += sizeof(PVOID);
				}

				if (!pProcessKernelCallbackArray)
				{
					*szNeeded = CallbackQuota - zeroHandles;
					goto cleanup;
				}
				else
				{
					*szNeeded = CallbackQuota - zeroHandles;
					pProcessKernelCallbackArray->Array = ProcessNotifyCallbackArray;
					pProcessKernelCallbackArray->CallbackQuota = CallbackQuota - zeroHandles;
				}

				for (i = 0; i < 8 * maxCallbacks; i += sizeof(PVOID))
				{
					callbackHandle = (ULONG64)((*(ULONG64*)(ULONG64*)((ULONG_PTR)ProcessNotifyCallbackArray + i)));
					if (callbackHandle != 0)
					{
						Callback = *(PULONG64)(callbackHandle & 0xfffffffffffffff8);
						moduleNumber = KBlast_GetKernelCallbackModuleNumber((PVOID)Callback, nModules, pAuxModuleExtendedInfo);
						if (moduleNumber != 0xffffffff)
						{
							// callback main info
							pProcessKernelCallbackArray->CallbackInformation[structCounter].CallbackHandle = callbackHandle;
							pProcessKernelCallbackArray->CallbackInformation[structCounter].PointerToHandle = (PVOID)((ULONG64)ProcessNotifyCallbackArray + i);
							pProcessKernelCallbackArray->CallbackInformation[structCounter].CallbackFunctionPointer = (PVOID)Callback;

							// callback module owner info
							pProcessKernelCallbackArray->CallbackInformation[structCounter].ModuleInformation.ModuleBase = pAuxModuleExtendedInfo[moduleNumber].BasicInfo.ImageBase;
							pProcessKernelCallbackArray->CallbackInformation[structCounter].ModuleInformation.ModuleFileNameOffset = pAuxModuleExtendedInfo[moduleNumber].FileNameOffset;
							pProcessKernelCallbackArray->CallbackInformation[structCounter].ModuleInformation.ModuleImageSize = pAuxModuleExtendedInfo[moduleNumber].ImageSize;
							if (strlen((const char*)pAuxModuleExtendedInfo[moduleNumber].FullPathName) < AUX_KLIB_MODULE_PATH_LEN)
							{
								strcpy(pProcessKernelCallbackArray->CallbackInformation[structCounter].ModuleInformation.ModuleFullPathName, (const char*)pAuxModuleExtendedInfo[moduleNumber].FullPathName);
							}

							structCounter++;
						}
					}
				}
			}
		}
	}

cleanup:

	if (pAuxModuleExtendedInfo != 0)
	{
		ExFreePoolWithTag(pAuxModuleExtendedInfo, POOL_TAG);
	}
	if (!NT_SUCCESS(status))
	{
		pProcessKernelCallbackArray->Array = 0;
	}

	structCounter = 0;

	return status;

}



NTSTATUS KBlast_EnumProcessCallbacks(IN ULONG szAvailable, IN CALLBACK_TYPE cType, OUT PVOID pOutBuf)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PVOID pStorage = 0;
	ULONG i = 0, szNeeded = 0;
	PROCESS_KERNEL_CALLBACK_ARRAY* outBuffer = (PROCESS_KERNEL_CALLBACK_ARRAY*)pOutBuf;
	PROCESS_KERNEL_CALLBACK_ARRAY pInfo = { 0 };

	if (szAvailable >= sizeof(PROCESS_KERNEL_CALLBACK_ARRAY))
	{
		pStorage = KBlast_GetCallbackStoragePointer(cType);
		if (pStorage != 0)
		{
			if ((cType == ARRAY_PROCESS) || (cType == ARRAY_THREAD) || (cType == ARRAY_IMAGE))
			{
				status = KBlast_GetCallbackArrayInformation(pStorage, &szNeeded, &pInfo);
				if (NT_SUCCESS(status))
				{
					outBuffer->Array = pInfo.Array;
					outBuffer->CallbackQuota = pInfo.CallbackQuota;
					for (i = 0; i < pInfo.CallbackQuota; i++)
					{
						outBuffer->CallbackInformation[i].CallbackFunctionPointer = pInfo.CallbackInformation[i].CallbackFunctionPointer;
						outBuffer->CallbackInformation[i].CallbackHandle = pInfo.CallbackInformation[i].CallbackHandle;
						outBuffer->CallbackInformation[i].PointerToHandle = pInfo.CallbackInformation[i].PointerToHandle;

						outBuffer->CallbackInformation[i].ModuleInformation.ModuleBase = pInfo.CallbackInformation[i].ModuleInformation.ModuleBase;
						outBuffer->CallbackInformation[i].ModuleInformation.ModuleFileNameOffset = pInfo.CallbackInformation[i].ModuleInformation.ModuleFileNameOffset;
						outBuffer->CallbackInformation[i].ModuleInformation.ModuleImageSize = pInfo.CallbackInformation[i].ModuleInformation.ModuleImageSize;
						strcpy(outBuffer->CallbackInformation[i].ModuleInformation.ModuleFullPathName, pInfo.CallbackInformation[i].ModuleInformation.ModuleFullPathName);
					}

					RtlZeroMemory(&pInfo, sizeof(PROCESS_KERNEL_CALLBACK_ARRAY));

				}
			}
			else if ((cType == LISTENTRY_REGISTRY) || (cType == LISTENTRY_OBJECT))
			{
				status = KBlast_GetCallbackListEntryInformation(pStorage, &pInfo);
				if (NT_SUCCESS(status))
				{
					outBuffer->Array = pInfo.Array;
					outBuffer->CallbackQuota = pInfo.CallbackQuota;
					for (i = 0; i < pInfo.CallbackQuota; i++)
					{
						outBuffer->CallbackInformation[i].CallbackFunctionPointer = pInfo.CallbackInformation[i].CallbackFunctionPointer;
						outBuffer->CallbackInformation[i].CallbackHandle = pInfo.CallbackInformation[i].CallbackHandle;
						outBuffer->CallbackInformation[i].PointerToHandle = pInfo.CallbackInformation[i].PointerToHandle;

						outBuffer->CallbackInformation[i].ModuleInformation.ModuleBase = pInfo.CallbackInformation[i].ModuleInformation.ModuleBase;
						outBuffer->CallbackInformation[i].ModuleInformation.ModuleFileNameOffset = pInfo.CallbackInformation[i].ModuleInformation.ModuleFileNameOffset;
						outBuffer->CallbackInformation[i].ModuleInformation.ModuleImageSize = pInfo.CallbackInformation[i].ModuleInformation.ModuleImageSize;
						strcpy(outBuffer->CallbackInformation[i].ModuleInformation.ModuleFullPathName, pInfo.CallbackInformation[i].ModuleInformation.ModuleFullPathName);
					}

					RtlZeroMemory(&pInfo, sizeof(PROCESS_KERNEL_CALLBACK_ARRAY));

				}
			}
		}
	}
	else
	{
		status = STATUS_BUFFER_TOO_SMALL;
	}

	return status;

}