/* libcudart shim: cudaX -> musaX
 * GENERATED -- ABI-identical tail-call jumps.
 */
	.text
	.globl cudaArrayGetInfo
	.type cudaArrayGetInfo, @function
cudaArrayGetInfo:
	jmp musaArrayGetInfo@PLT
	.size cudaArrayGetInfo, .-cudaArrayGetInfo
	.globl cudaBindSurfaceToArray
	.type cudaBindSurfaceToArray, @function
cudaBindSurfaceToArray:
	jmp musaBindSurfaceToArray@PLT
	.size cudaBindSurfaceToArray, .-cudaBindSurfaceToArray
	.globl cudaBindTexture
	.type cudaBindTexture, @function
cudaBindTexture:
	jmp musaBindTexture@PLT
	.size cudaBindTexture, .-cudaBindTexture
	.globl cudaBindTexture2D
	.type cudaBindTexture2D, @function
cudaBindTexture2D:
	jmp musaBindTexture2D@PLT
	.size cudaBindTexture2D, .-cudaBindTexture2D
	.globl cudaBindTextureToArray
	.type cudaBindTextureToArray, @function
cudaBindTextureToArray:
	jmp musaBindTextureToArray@PLT
	.size cudaBindTextureToArray, .-cudaBindTextureToArray
	.globl cudaBindTextureToMipmappedArray
	.type cudaBindTextureToMipmappedArray, @function
cudaBindTextureToMipmappedArray:
	jmp musaBindTextureToMipmappedArray@PLT
	.size cudaBindTextureToMipmappedArray, .-cudaBindTextureToMipmappedArray
	.globl cudaChooseDevice
	.type cudaChooseDevice, @function
cudaChooseDevice:
	jmp musaChooseDevice@PLT
	.size cudaChooseDevice, .-cudaChooseDevice
	.globl cudaCreateChannelDesc
	.type cudaCreateChannelDesc, @function
cudaCreateChannelDesc:
	jmp musaCreateChannelDesc@PLT
	.size cudaCreateChannelDesc, .-cudaCreateChannelDesc
	.globl cudaCreateSurfaceObject
	.type cudaCreateSurfaceObject, @function
cudaCreateSurfaceObject:
	jmp musaCreateSurfaceObject@PLT
	.size cudaCreateSurfaceObject, .-cudaCreateSurfaceObject
	.globl cudaCreateTextureObject
	.type cudaCreateTextureObject, @function
cudaCreateTextureObject:
	jmp musaCreateTextureObject@PLT
	.size cudaCreateTextureObject, .-cudaCreateTextureObject
	.globl cudaCtxResetPersistingL2Cache
	.type cudaCtxResetPersistingL2Cache, @function
cudaCtxResetPersistingL2Cache:
	jmp musaCtxResetPersistingL2Cache@PLT
	.size cudaCtxResetPersistingL2Cache, .-cudaCtxResetPersistingL2Cache
	.globl cudaDestroySurfaceObject
	.type cudaDestroySurfaceObject, @function
cudaDestroySurfaceObject:
	jmp musaDestroySurfaceObject@PLT
	.size cudaDestroySurfaceObject, .-cudaDestroySurfaceObject
	.globl cudaDestroyTextureObject
	.type cudaDestroyTextureObject, @function
cudaDestroyTextureObject:
	jmp musaDestroyTextureObject@PLT
	.size cudaDestroyTextureObject, .-cudaDestroyTextureObject
	.globl cudaDeviceCanAccessPeer
	.type cudaDeviceCanAccessPeer, @function
cudaDeviceCanAccessPeer:
	jmp musaDeviceCanAccessPeer@PLT
	.size cudaDeviceCanAccessPeer, .-cudaDeviceCanAccessPeer
	.globl cudaDeviceDisablePeerAccess
	.type cudaDeviceDisablePeerAccess, @function
cudaDeviceDisablePeerAccess:
	jmp musaDeviceDisablePeerAccess@PLT
	.size cudaDeviceDisablePeerAccess, .-cudaDeviceDisablePeerAccess
	.globl cudaDeviceEnablePeerAccess
	.type cudaDeviceEnablePeerAccess, @function
cudaDeviceEnablePeerAccess:
	jmp musaDeviceEnablePeerAccess@PLT
	.size cudaDeviceEnablePeerAccess, .-cudaDeviceEnablePeerAccess
	.globl cudaDeviceGetAttribute
	.type cudaDeviceGetAttribute, @function
cudaDeviceGetAttribute:
	jmp musaDeviceGetAttribute@PLT
	.size cudaDeviceGetAttribute, .-cudaDeviceGetAttribute
	.globl cudaDeviceGetByPCIBusId
	.type cudaDeviceGetByPCIBusId, @function
cudaDeviceGetByPCIBusId:
	jmp musaDeviceGetByPCIBusId@PLT
	.size cudaDeviceGetByPCIBusId, .-cudaDeviceGetByPCIBusId
	.globl cudaDeviceGetCacheConfig
	.type cudaDeviceGetCacheConfig, @function
cudaDeviceGetCacheConfig:
	jmp musaDeviceGetCacheConfig@PLT
	.size cudaDeviceGetCacheConfig, .-cudaDeviceGetCacheConfig
	.globl cudaDeviceGetLimit
	.type cudaDeviceGetLimit, @function
cudaDeviceGetLimit:
	jmp musaDeviceGetLimit@PLT
	.size cudaDeviceGetLimit, .-cudaDeviceGetLimit
	.globl cudaDeviceGetP2PAttribute
	.type cudaDeviceGetP2PAttribute, @function
cudaDeviceGetP2PAttribute:
	jmp musaDeviceGetP2PAttribute@PLT
	.size cudaDeviceGetP2PAttribute, .-cudaDeviceGetP2PAttribute
	.globl cudaDeviceGetPCIBusId
	.type cudaDeviceGetPCIBusId, @function
cudaDeviceGetPCIBusId:
	jmp musaDeviceGetPCIBusId@PLT
	.size cudaDeviceGetPCIBusId, .-cudaDeviceGetPCIBusId
	.globl cudaDeviceGetSharedMemConfig
	.type cudaDeviceGetSharedMemConfig, @function
cudaDeviceGetSharedMemConfig:
	jmp musaDeviceGetSharedMemConfig@PLT
	.size cudaDeviceGetSharedMemConfig, .-cudaDeviceGetSharedMemConfig
	.globl cudaDeviceGetStreamPriorityRange
	.type cudaDeviceGetStreamPriorityRange, @function
cudaDeviceGetStreamPriorityRange:
	jmp musaDeviceGetStreamPriorityRange@PLT
	.size cudaDeviceGetStreamPriorityRange, .-cudaDeviceGetStreamPriorityRange
	.globl cudaDeviceReset
	.type cudaDeviceReset, @function
cudaDeviceReset:
	jmp musaDeviceReset@PLT
	.size cudaDeviceReset, .-cudaDeviceReset
	.globl cudaDeviceSetCacheConfig
	.type cudaDeviceSetCacheConfig, @function
cudaDeviceSetCacheConfig:
	jmp musaDeviceSetCacheConfig@PLT
	.size cudaDeviceSetCacheConfig, .-cudaDeviceSetCacheConfig
	.globl cudaDeviceSetLimit
	.type cudaDeviceSetLimit, @function
cudaDeviceSetLimit:
	jmp musaDeviceSetLimit@PLT
	.size cudaDeviceSetLimit, .-cudaDeviceSetLimit
	.globl cudaDeviceSetSharedMemConfig
	.type cudaDeviceSetSharedMemConfig, @function
cudaDeviceSetSharedMemConfig:
	jmp musaDeviceSetSharedMemConfig@PLT
	.size cudaDeviceSetSharedMemConfig, .-cudaDeviceSetSharedMemConfig
	.globl cudaDeviceSynchronize
	.type cudaDeviceSynchronize, @function
cudaDeviceSynchronize:
	jmp musaDeviceSynchronize@PLT
	.size cudaDeviceSynchronize, .-cudaDeviceSynchronize
	.globl cudaDriverGetVersion
	.type cudaDriverGetVersion, @function
cudaDriverGetVersion:
	jmp musaDriverGetVersion@PLT
	.size cudaDriverGetVersion, .-cudaDriverGetVersion
	.globl cudaEventCreate
	.type cudaEventCreate, @function
cudaEventCreate:
	jmp musaEventCreate@PLT
	.size cudaEventCreate, .-cudaEventCreate
	.globl cudaEventCreateWithFlags
	.type cudaEventCreateWithFlags, @function
cudaEventCreateWithFlags:
	jmp musaEventCreateWithFlags@PLT
	.size cudaEventCreateWithFlags, .-cudaEventCreateWithFlags
	.globl cudaEventDestroy
	.type cudaEventDestroy, @function
cudaEventDestroy:
	jmp musaEventDestroy@PLT
	.size cudaEventDestroy, .-cudaEventDestroy
	.globl cudaEventElapsedTime
	.type cudaEventElapsedTime, @function
cudaEventElapsedTime:
	jmp musaEventElapsedTime@PLT
	.size cudaEventElapsedTime, .-cudaEventElapsedTime
	.globl cudaEventQuery
	.type cudaEventQuery, @function
cudaEventQuery:
	jmp musaEventQuery@PLT
	.size cudaEventQuery, .-cudaEventQuery
	.globl cudaEventRecord
	.type cudaEventRecord, @function
cudaEventRecord:
	jmp musaEventRecord@PLT
	.size cudaEventRecord, .-cudaEventRecord
	.globl cudaEventRecordWithFlags
	.type cudaEventRecordWithFlags, @function
cudaEventRecordWithFlags:
	jmp musaEventRecordWithFlags@PLT
	.size cudaEventRecordWithFlags, .-cudaEventRecordWithFlags
	.globl cudaEventSynchronize
	.type cudaEventSynchronize, @function
cudaEventSynchronize:
	jmp musaEventSynchronize@PLT
	.size cudaEventSynchronize, .-cudaEventSynchronize
	.globl cudaFree
	.type cudaFree, @function
cudaFree:
	jmp musaFree@PLT
	.size cudaFree, .-cudaFree
	.globl cudaFreeArray
	.type cudaFreeArray, @function
cudaFreeArray:
	jmp musaFreeArray@PLT
	.size cudaFreeArray, .-cudaFreeArray
	.globl cudaFreeHost
	.type cudaFreeHost, @function
cudaFreeHost:
	jmp musaFreeHost@PLT
	.size cudaFreeHost, .-cudaFreeHost
	.globl cudaFreeMipmappedArray
	.type cudaFreeMipmappedArray, @function
cudaFreeMipmappedArray:
	jmp musaFreeMipmappedArray@PLT
	.size cudaFreeMipmappedArray, .-cudaFreeMipmappedArray
	.globl cudaFuncGetAttributes
	.type cudaFuncGetAttributes, @function
cudaFuncGetAttributes:
	jmp musaFuncGetAttributes@PLT
	.size cudaFuncGetAttributes, .-cudaFuncGetAttributes
	.globl cudaFuncSetAttribute
	.type cudaFuncSetAttribute, @function
cudaFuncSetAttribute:
	jmp musaFuncSetAttribute@PLT
	.size cudaFuncSetAttribute, .-cudaFuncSetAttribute
	.globl cudaFuncSetCacheConfig
	.type cudaFuncSetCacheConfig, @function
cudaFuncSetCacheConfig:
	jmp musaFuncSetCacheConfig@PLT
	.size cudaFuncSetCacheConfig, .-cudaFuncSetCacheConfig
	.globl cudaFuncSetSharedMemConfig
	.type cudaFuncSetSharedMemConfig, @function
cudaFuncSetSharedMemConfig:
	jmp musaFuncSetSharedMemConfig@PLT
	.size cudaFuncSetSharedMemConfig, .-cudaFuncSetSharedMemConfig
	.globl cudaGLSetGLDevice
	.type cudaGLSetGLDevice, @function
cudaGLSetGLDevice:
	jmp musaGLSetGLDevice@PLT
	.size cudaGLSetGLDevice, .-cudaGLSetGLDevice
	.globl cudaGetChannelDesc
	.type cudaGetChannelDesc, @function
cudaGetChannelDesc:
	jmp musaGetChannelDesc@PLT
	.size cudaGetChannelDesc, .-cudaGetChannelDesc
	.globl cudaGetDevice
	.type cudaGetDevice, @function
cudaGetDevice:
	jmp musaGetDevice@PLT
	.size cudaGetDevice, .-cudaGetDevice
	.globl cudaGetDeviceCount
	.type cudaGetDeviceCount, @function
cudaGetDeviceCount:
	jmp musaGetDeviceCount@PLT
	.size cudaGetDeviceCount, .-cudaGetDeviceCount
	.globl cudaGetDeviceFlags
	.type cudaGetDeviceFlags, @function
cudaGetDeviceFlags:
	jmp musaGetDeviceFlags@PLT
	.size cudaGetDeviceFlags, .-cudaGetDeviceFlags
	.globl cudaGetDeviceProperties
	.type cudaGetDeviceProperties, @function
cudaGetDeviceProperties:
	jmp musaGetDeviceProperties@PLT
	.size cudaGetDeviceProperties, .-cudaGetDeviceProperties
	.globl cudaGetErrorName
	.type cudaGetErrorName, @function
cudaGetErrorName:
	jmp musaGetErrorName@PLT
	.size cudaGetErrorName, .-cudaGetErrorName
	.globl cudaGetErrorString
	.type cudaGetErrorString, @function
cudaGetErrorString:
	jmp musaGetErrorString@PLT
	.size cudaGetErrorString, .-cudaGetErrorString
	.globl cudaGetExportTable
	.type cudaGetExportTable, @function
cudaGetExportTable:
	jmp musaGetExportTable@PLT
	.size cudaGetExportTable, .-cudaGetExportTable
	.globl cudaGetLastError
	.type cudaGetLastError, @function
cudaGetLastError:
	jmp musaGetLastError@PLT
	.size cudaGetLastError, .-cudaGetLastError
	.globl cudaGetMipmappedArrayLevel
	.type cudaGetMipmappedArrayLevel, @function
cudaGetMipmappedArrayLevel:
	jmp musaGetMipmappedArrayLevel@PLT
	.size cudaGetMipmappedArrayLevel, .-cudaGetMipmappedArrayLevel
	.globl cudaGetSurfaceObjectResourceDesc
	.type cudaGetSurfaceObjectResourceDesc, @function
cudaGetSurfaceObjectResourceDesc:
	jmp musaGetSurfaceObjectResourceDesc@PLT
	.size cudaGetSurfaceObjectResourceDesc, .-cudaGetSurfaceObjectResourceDesc
	.globl cudaGetSurfaceReference
	.type cudaGetSurfaceReference, @function
cudaGetSurfaceReference:
	jmp musaGetSurfaceReference@PLT
	.size cudaGetSurfaceReference, .-cudaGetSurfaceReference
	.globl cudaGetSymbolAddress
	.type cudaGetSymbolAddress, @function
cudaGetSymbolAddress:
	jmp musaGetSymbolAddress@PLT
	.size cudaGetSymbolAddress, .-cudaGetSymbolAddress
	.globl cudaGetSymbolSize
	.type cudaGetSymbolSize, @function
cudaGetSymbolSize:
	jmp musaGetSymbolSize@PLT
	.size cudaGetSymbolSize, .-cudaGetSymbolSize
	.globl cudaGetTextureAlignmentOffset
	.type cudaGetTextureAlignmentOffset, @function
cudaGetTextureAlignmentOffset:
	jmp musaGetTextureAlignmentOffset@PLT
	.size cudaGetTextureAlignmentOffset, .-cudaGetTextureAlignmentOffset
	.globl cudaGetTextureObjectResourceDesc
	.type cudaGetTextureObjectResourceDesc, @function
cudaGetTextureObjectResourceDesc:
	jmp musaGetTextureObjectResourceDesc@PLT
	.size cudaGetTextureObjectResourceDesc, .-cudaGetTextureObjectResourceDesc
	.globl cudaGetTextureObjectResourceViewDesc
	.type cudaGetTextureObjectResourceViewDesc, @function
cudaGetTextureObjectResourceViewDesc:
	jmp musaGetTextureObjectResourceViewDesc@PLT
	.size cudaGetTextureObjectResourceViewDesc, .-cudaGetTextureObjectResourceViewDesc
	.globl cudaGetTextureObjectTextureDesc
	.type cudaGetTextureObjectTextureDesc, @function
cudaGetTextureObjectTextureDesc:
	jmp musaGetTextureObjectTextureDesc@PLT
	.size cudaGetTextureObjectTextureDesc, .-cudaGetTextureObjectTextureDesc
	.globl cudaGetTextureReference
	.type cudaGetTextureReference, @function
cudaGetTextureReference:
	jmp musaGetTextureReference@PLT
	.size cudaGetTextureReference, .-cudaGetTextureReference
	.globl cudaGraphAddChildGraphNode
	.type cudaGraphAddChildGraphNode, @function
cudaGraphAddChildGraphNode:
	jmp musaGraphAddChildGraphNode@PLT
	.size cudaGraphAddChildGraphNode, .-cudaGraphAddChildGraphNode
	.globl cudaGraphAddEmptyNode
	.type cudaGraphAddEmptyNode, @function
cudaGraphAddEmptyNode:
	jmp musaGraphAddEmptyNode@PLT
	.size cudaGraphAddEmptyNode, .-cudaGraphAddEmptyNode
	.globl cudaGraphAddEventRecordNode
	.type cudaGraphAddEventRecordNode, @function
cudaGraphAddEventRecordNode:
	jmp musaGraphAddEventRecordNode@PLT
	.size cudaGraphAddEventRecordNode, .-cudaGraphAddEventRecordNode
	.globl cudaGraphAddEventWaitNode
	.type cudaGraphAddEventWaitNode, @function
cudaGraphAddEventWaitNode:
	jmp musaGraphAddEventWaitNode@PLT
	.size cudaGraphAddEventWaitNode, .-cudaGraphAddEventWaitNode
	.globl cudaGraphAddHostNode
	.type cudaGraphAddHostNode, @function
cudaGraphAddHostNode:
	jmp musaGraphAddHostNode@PLT
	.size cudaGraphAddHostNode, .-cudaGraphAddHostNode
	.globl cudaGraphAddKernelNode
	.type cudaGraphAddKernelNode, @function
cudaGraphAddKernelNode:
	jmp musaGraphAddKernelNode@PLT
	.size cudaGraphAddKernelNode, .-cudaGraphAddKernelNode
	.globl cudaGraphAddMemcpyNode1D
	.type cudaGraphAddMemcpyNode1D, @function
cudaGraphAddMemcpyNode1D:
	jmp musaGraphAddMemcpyNode1D@PLT
	.size cudaGraphAddMemcpyNode1D, .-cudaGraphAddMemcpyNode1D
	.globl cudaGraphAddMemsetNode
	.type cudaGraphAddMemsetNode, @function
cudaGraphAddMemsetNode:
	jmp musaGraphAddMemsetNode@PLT
	.size cudaGraphAddMemsetNode, .-cudaGraphAddMemsetNode
	.globl cudaGraphClone
	.type cudaGraphClone, @function
cudaGraphClone:
	jmp musaGraphClone@PLT
	.size cudaGraphClone, .-cudaGraphClone
	.globl cudaGraphCreate
	.type cudaGraphCreate, @function
cudaGraphCreate:
	jmp musaGraphCreate@PLT
	.size cudaGraphCreate, .-cudaGraphCreate
	.globl cudaGraphDestroy
	.type cudaGraphDestroy, @function
cudaGraphDestroy:
	jmp musaGraphDestroy@PLT
	.size cudaGraphDestroy, .-cudaGraphDestroy
	.globl cudaGraphExecDestroy
	.type cudaGraphExecDestroy, @function
cudaGraphExecDestroy:
	jmp musaGraphExecDestroy@PLT
	.size cudaGraphExecDestroy, .-cudaGraphExecDestroy
	.globl cudaGraphExecKernelNodeSetParams
	.type cudaGraphExecKernelNodeSetParams, @function
cudaGraphExecKernelNodeSetParams:
	jmp musaGraphExecKernelNodeSetParams@PLT
	.size cudaGraphExecKernelNodeSetParams, .-cudaGraphExecKernelNodeSetParams
	.globl cudaGraphExecUpdate
	.type cudaGraphExecUpdate, @function
cudaGraphExecUpdate:
	jmp musaGraphExecUpdate@PLT
	.size cudaGraphExecUpdate, .-cudaGraphExecUpdate
	.globl cudaGraphGetNodes
	.type cudaGraphGetNodes, @function
cudaGraphGetNodes:
	jmp musaGraphGetNodes@PLT
	.size cudaGraphGetNodes, .-cudaGraphGetNodes
	.globl cudaGraphGetRootNodes
	.type cudaGraphGetRootNodes, @function
cudaGraphGetRootNodes:
	jmp musaGraphGetRootNodes@PLT
	.size cudaGraphGetRootNodes, .-cudaGraphGetRootNodes
	.globl cudaGraphHostNodeGetParams
	.type cudaGraphHostNodeGetParams, @function
cudaGraphHostNodeGetParams:
	jmp musaGraphHostNodeGetParams@PLT
	.size cudaGraphHostNodeGetParams, .-cudaGraphHostNodeGetParams
	.globl cudaGraphHostNodeSetParams
	.type cudaGraphHostNodeSetParams, @function
cudaGraphHostNodeSetParams:
	jmp musaGraphHostNodeSetParams@PLT
	.size cudaGraphHostNodeSetParams, .-cudaGraphHostNodeSetParams
	.globl cudaGraphInstantiate
	.type cudaGraphInstantiate, @function
cudaGraphInstantiate:
	jmp musaGraphInstantiate@PLT
	.size cudaGraphInstantiate, .-cudaGraphInstantiate
	.globl cudaGraphKernelNodeCopyAttributes
	.type cudaGraphKernelNodeCopyAttributes, @function
cudaGraphKernelNodeCopyAttributes:
	jmp musaGraphKernelNodeCopyAttributes@PLT
	.size cudaGraphKernelNodeCopyAttributes, .-cudaGraphKernelNodeCopyAttributes
	.globl cudaGraphKernelNodeGetAttribute
	.type cudaGraphKernelNodeGetAttribute, @function
cudaGraphKernelNodeGetAttribute:
	jmp musaGraphKernelNodeGetAttribute@PLT
	.size cudaGraphKernelNodeGetAttribute, .-cudaGraphKernelNodeGetAttribute
	.globl cudaGraphKernelNodeGetParams
	.type cudaGraphKernelNodeGetParams, @function
cudaGraphKernelNodeGetParams:
	jmp musaGraphKernelNodeGetParams@PLT
	.size cudaGraphKernelNodeGetParams, .-cudaGraphKernelNodeGetParams
	.globl cudaGraphKernelNodeSetAttribute
	.type cudaGraphKernelNodeSetAttribute, @function
cudaGraphKernelNodeSetAttribute:
	jmp musaGraphKernelNodeSetAttribute@PLT
	.size cudaGraphKernelNodeSetAttribute, .-cudaGraphKernelNodeSetAttribute
	.globl cudaGraphKernelNodeSetParams
	.type cudaGraphKernelNodeSetParams, @function
cudaGraphKernelNodeSetParams:
	jmp musaGraphKernelNodeSetParams@PLT
	.size cudaGraphKernelNodeSetParams, .-cudaGraphKernelNodeSetParams
	.globl cudaGraphLaunch
	.type cudaGraphLaunch, @function
cudaGraphLaunch:
	jmp musaGraphLaunch@PLT
	.size cudaGraphLaunch, .-cudaGraphLaunch
	.globl cudaGraphMemcpyNodeGetParams
	.type cudaGraphMemcpyNodeGetParams, @function
cudaGraphMemcpyNodeGetParams:
	jmp musaGraphMemcpyNodeGetParams@PLT
	.size cudaGraphMemcpyNodeGetParams, .-cudaGraphMemcpyNodeGetParams
	.globl cudaGraphMemcpyNodeSetParams
	.type cudaGraphMemcpyNodeSetParams, @function
cudaGraphMemcpyNodeSetParams:
	jmp musaGraphMemcpyNodeSetParams@PLT
	.size cudaGraphMemcpyNodeSetParams, .-cudaGraphMemcpyNodeSetParams
	.globl cudaGraphMemsetNodeGetParams
	.type cudaGraphMemsetNodeGetParams, @function
cudaGraphMemsetNodeGetParams:
	jmp musaGraphMemsetNodeGetParams@PLT
	.size cudaGraphMemsetNodeGetParams, .-cudaGraphMemsetNodeGetParams
	.globl cudaGraphMemsetNodeSetParams
	.type cudaGraphMemsetNodeSetParams, @function
cudaGraphMemsetNodeSetParams:
	jmp musaGraphMemsetNodeSetParams@PLT
	.size cudaGraphMemsetNodeSetParams, .-cudaGraphMemsetNodeSetParams
	.globl cudaGraphNodeFindInClone
	.type cudaGraphNodeFindInClone, @function
cudaGraphNodeFindInClone:
	jmp musaGraphNodeFindInClone@PLT
	.size cudaGraphNodeFindInClone, .-cudaGraphNodeFindInClone
	.globl cudaGraphNodeGetType
	.type cudaGraphNodeGetType, @function
cudaGraphNodeGetType:
	jmp musaGraphNodeGetType@PLT
	.size cudaGraphNodeGetType, .-cudaGraphNodeGetType
	.globl cudaGraphicsGLRegisterBuffer
	.type cudaGraphicsGLRegisterBuffer, @function
cudaGraphicsGLRegisterBuffer:
	jmp musaGraphicsGLRegisterBuffer@PLT
	.size cudaGraphicsGLRegisterBuffer, .-cudaGraphicsGLRegisterBuffer
	.globl cudaGraphicsMapResources
	.type cudaGraphicsMapResources, @function
cudaGraphicsMapResources:
	jmp musaGraphicsMapResources@PLT
	.size cudaGraphicsMapResources, .-cudaGraphicsMapResources
	.globl cudaGraphicsResourceGetMappedPointer
	.type cudaGraphicsResourceGetMappedPointer, @function
cudaGraphicsResourceGetMappedPointer:
	jmp musaGraphicsResourceGetMappedPointer@PLT
	.size cudaGraphicsResourceGetMappedPointer, .-cudaGraphicsResourceGetMappedPointer
	.globl cudaGraphicsResourceSetMapFlags
	.type cudaGraphicsResourceSetMapFlags, @function
cudaGraphicsResourceSetMapFlags:
	jmp musaGraphicsResourceSetMapFlags@PLT
	.size cudaGraphicsResourceSetMapFlags, .-cudaGraphicsResourceSetMapFlags
	.globl cudaGraphicsUnmapResources
	.type cudaGraphicsUnmapResources, @function
cudaGraphicsUnmapResources:
	jmp musaGraphicsUnmapResources@PLT
	.size cudaGraphicsUnmapResources, .-cudaGraphicsUnmapResources
	.globl cudaGraphicsUnregisterResource
	.type cudaGraphicsUnregisterResource, @function
cudaGraphicsUnregisterResource:
	jmp musaGraphicsUnregisterResource@PLT
	.size cudaGraphicsUnregisterResource, .-cudaGraphicsUnregisterResource
	.globl cudaHostAlloc
	.type cudaHostAlloc, @function
cudaHostAlloc:
	jmp musaHostAlloc@PLT
	.size cudaHostAlloc, .-cudaHostAlloc
	.globl cudaHostGetDevicePointer
	.type cudaHostGetDevicePointer, @function
cudaHostGetDevicePointer:
	jmp musaHostGetDevicePointer@PLT
	.size cudaHostGetDevicePointer, .-cudaHostGetDevicePointer
	.globl cudaHostGetFlags
	.type cudaHostGetFlags, @function
cudaHostGetFlags:
	jmp musaHostGetFlags@PLT
	.size cudaHostGetFlags, .-cudaHostGetFlags
	.globl cudaHostRegister
	.type cudaHostRegister, @function
cudaHostRegister:
	jmp musaHostRegister@PLT
	.size cudaHostRegister, .-cudaHostRegister
	.globl cudaHostUnregister
	.type cudaHostUnregister, @function
cudaHostUnregister:
	jmp musaHostUnregister@PLT
	.size cudaHostUnregister, .-cudaHostUnregister
	.globl cudaIpcCloseMemHandle
	.type cudaIpcCloseMemHandle, @function
cudaIpcCloseMemHandle:
	jmp musaIpcCloseMemHandle@PLT
	.size cudaIpcCloseMemHandle, .-cudaIpcCloseMemHandle
	.globl cudaIpcGetEventHandle
	.type cudaIpcGetEventHandle, @function
cudaIpcGetEventHandle:
	jmp musaIpcGetEventHandle@PLT
	.size cudaIpcGetEventHandle, .-cudaIpcGetEventHandle
	.globl cudaIpcGetMemHandle
	.type cudaIpcGetMemHandle, @function
cudaIpcGetMemHandle:
	jmp musaIpcGetMemHandle@PLT
	.size cudaIpcGetMemHandle, .-cudaIpcGetMemHandle
	.globl cudaIpcOpenEventHandle
	.type cudaIpcOpenEventHandle, @function
cudaIpcOpenEventHandle:
	jmp musaIpcOpenEventHandle@PLT
	.size cudaIpcOpenEventHandle, .-cudaIpcOpenEventHandle
	.globl cudaIpcOpenMemHandle
	.type cudaIpcOpenMemHandle, @function
cudaIpcOpenMemHandle:
	jmp musaIpcOpenMemHandle@PLT
	.size cudaIpcOpenMemHandle, .-cudaIpcOpenMemHandle
	.globl cudaLaunchCooperativeKernel
	.type cudaLaunchCooperativeKernel, @function
cudaLaunchCooperativeKernel:
	jmp musaLaunchCooperativeKernel@PLT
	.size cudaLaunchCooperativeKernel, .-cudaLaunchCooperativeKernel
	.globl cudaLaunchCooperativeKernelMultiDevice
	.type cudaLaunchCooperativeKernelMultiDevice, @function
cudaLaunchCooperativeKernelMultiDevice:
	jmp musaLaunchCooperativeKernelMultiDevice@PLT
	.size cudaLaunchCooperativeKernelMultiDevice, .-cudaLaunchCooperativeKernelMultiDevice
	.globl cudaLaunchHostFunc
	.type cudaLaunchHostFunc, @function
cudaLaunchHostFunc:
	jmp musaLaunchHostFunc@PLT
	.size cudaLaunchHostFunc, .-cudaLaunchHostFunc
	.globl cudaLaunchKernel
	.type cudaLaunchKernel, @function
cudaLaunchKernel:
	jmp musaLaunchKernel@PLT
	.size cudaLaunchKernel, .-cudaLaunchKernel
	.globl cudaLaunchKernelExC
	.type cudaLaunchKernelExC, @function
cudaLaunchKernelExC:
	jmp musaLaunchKernelExC@PLT
	.size cudaLaunchKernelExC, .-cudaLaunchKernelExC
	.globl cudaMalloc
	.type cudaMalloc, @function
cudaMalloc:
	jmp musaMalloc@PLT
	.size cudaMalloc, .-cudaMalloc
	.globl cudaMalloc3D
	.type cudaMalloc3D, @function
cudaMalloc3D:
	jmp musaMalloc3D@PLT
	.size cudaMalloc3D, .-cudaMalloc3D
	.globl cudaMalloc3DArray
	.type cudaMalloc3DArray, @function
cudaMalloc3DArray:
	jmp musaMalloc3DArray@PLT
	.size cudaMalloc3DArray, .-cudaMalloc3DArray
	.globl cudaMallocArray
	.type cudaMallocArray, @function
cudaMallocArray:
	jmp musaMallocArray@PLT
	.size cudaMallocArray, .-cudaMallocArray
	.globl cudaMallocFromPoolAsync
	.type cudaMallocFromPoolAsync, @function
cudaMallocFromPoolAsync:
	jmp musaMallocFromPoolAsync@PLT
	.size cudaMallocFromPoolAsync, .-cudaMallocFromPoolAsync
	.globl cudaMallocHost
	.type cudaMallocHost, @function
cudaMallocHost:
	jmp musaMallocHost@PLT
	.size cudaMallocHost, .-cudaMallocHost
	.globl cudaMallocManaged
	.type cudaMallocManaged, @function
cudaMallocManaged:
	jmp musaMallocManaged@PLT
	.size cudaMallocManaged, .-cudaMallocManaged
	.globl cudaMallocMipmappedArray
	.type cudaMallocMipmappedArray, @function
cudaMallocMipmappedArray:
	jmp musaMallocMipmappedArray@PLT
	.size cudaMallocMipmappedArray, .-cudaMallocMipmappedArray
	.globl cudaMallocPitch
	.type cudaMallocPitch, @function
cudaMallocPitch:
	jmp musaMallocPitch@PLT
	.size cudaMallocPitch, .-cudaMallocPitch
	.globl cudaMemAdvise
	.type cudaMemAdvise, @function
cudaMemAdvise:
	jmp musaMemAdvise@PLT
	.size cudaMemAdvise, .-cudaMemAdvise
	.globl cudaMemGetInfo
	.type cudaMemGetInfo, @function
cudaMemGetInfo:
	jmp musaMemGetInfo@PLT
	.size cudaMemGetInfo, .-cudaMemGetInfo
	.globl cudaMemPrefetchAsync
	.type cudaMemPrefetchAsync, @function
cudaMemPrefetchAsync:
	jmp musaMemPrefetchAsync@PLT
	.size cudaMemPrefetchAsync, .-cudaMemPrefetchAsync
	.globl cudaMemRangeGetAttribute
	.type cudaMemRangeGetAttribute, @function
cudaMemRangeGetAttribute:
	jmp musaMemRangeGetAttribute@PLT
	.size cudaMemRangeGetAttribute, .-cudaMemRangeGetAttribute
	.globl cudaMemRangeGetAttributes
	.type cudaMemRangeGetAttributes, @function
cudaMemRangeGetAttributes:
	jmp musaMemRangeGetAttributes@PLT
	.size cudaMemRangeGetAttributes, .-cudaMemRangeGetAttributes
	.globl cudaMemcpy
	.type cudaMemcpy, @function
cudaMemcpy:
	jmp musaMemcpy@PLT
	.size cudaMemcpy, .-cudaMemcpy
	.globl cudaMemcpy2D
	.type cudaMemcpy2D, @function
cudaMemcpy2D:
	jmp musaMemcpy2D@PLT
	.size cudaMemcpy2D, .-cudaMemcpy2D
	.globl cudaMemcpy2DArrayToArray
	.type cudaMemcpy2DArrayToArray, @function
cudaMemcpy2DArrayToArray:
	jmp musaMemcpy2DArrayToArray@PLT
	.size cudaMemcpy2DArrayToArray, .-cudaMemcpy2DArrayToArray
	.globl cudaMemcpy2DAsync
	.type cudaMemcpy2DAsync, @function
cudaMemcpy2DAsync:
	jmp musaMemcpy2DAsync@PLT
	.size cudaMemcpy2DAsync, .-cudaMemcpy2DAsync
	.globl cudaMemcpy2DFromArray
	.type cudaMemcpy2DFromArray, @function
cudaMemcpy2DFromArray:
	jmp musaMemcpy2DFromArray@PLT
	.size cudaMemcpy2DFromArray, .-cudaMemcpy2DFromArray
	.globl cudaMemcpy2DFromArrayAsync
	.type cudaMemcpy2DFromArrayAsync, @function
cudaMemcpy2DFromArrayAsync:
	jmp musaMemcpy2DFromArrayAsync@PLT
	.size cudaMemcpy2DFromArrayAsync, .-cudaMemcpy2DFromArrayAsync
	.globl cudaMemcpy2DToArray
	.type cudaMemcpy2DToArray, @function
cudaMemcpy2DToArray:
	jmp musaMemcpy2DToArray@PLT
	.size cudaMemcpy2DToArray, .-cudaMemcpy2DToArray
	.globl cudaMemcpy2DToArrayAsync
	.type cudaMemcpy2DToArrayAsync, @function
cudaMemcpy2DToArrayAsync:
	jmp musaMemcpy2DToArrayAsync@PLT
	.size cudaMemcpy2DToArrayAsync, .-cudaMemcpy2DToArrayAsync
	.globl cudaMemcpy3D
	.type cudaMemcpy3D, @function
cudaMemcpy3D:
	jmp musaMemcpy3D@PLT
	.size cudaMemcpy3D, .-cudaMemcpy3D
	.globl cudaMemcpy3DAsync
	.type cudaMemcpy3DAsync, @function
cudaMemcpy3DAsync:
	jmp musaMemcpy3DAsync@PLT
	.size cudaMemcpy3DAsync, .-cudaMemcpy3DAsync
	.globl cudaMemcpyAsync
	.type cudaMemcpyAsync, @function
cudaMemcpyAsync:
	jmp musaMemcpyAsync@PLT
	.size cudaMemcpyAsync, .-cudaMemcpyAsync
	.globl cudaMemcpyFromArray
	.type cudaMemcpyFromArray, @function
cudaMemcpyFromArray:
	jmp musaMemcpyFromArray@PLT
	.size cudaMemcpyFromArray, .-cudaMemcpyFromArray
	.globl cudaMemcpyFromArrayAsync
	.type cudaMemcpyFromArrayAsync, @function
cudaMemcpyFromArrayAsync:
	jmp musaMemcpyFromArrayAsync@PLT
	.size cudaMemcpyFromArrayAsync, .-cudaMemcpyFromArrayAsync
	.globl cudaMemcpyFromSymbol
	.type cudaMemcpyFromSymbol, @function
cudaMemcpyFromSymbol:
	jmp musaMemcpyFromSymbol@PLT
	.size cudaMemcpyFromSymbol, .-cudaMemcpyFromSymbol
	.globl cudaMemcpyFromSymbolAsync
	.type cudaMemcpyFromSymbolAsync, @function
cudaMemcpyFromSymbolAsync:
	jmp musaMemcpyFromSymbolAsync@PLT
	.size cudaMemcpyFromSymbolAsync, .-cudaMemcpyFromSymbolAsync
	.globl cudaMemcpyPeer
	.type cudaMemcpyPeer, @function
cudaMemcpyPeer:
	jmp musaMemcpyPeer@PLT
	.size cudaMemcpyPeer, .-cudaMemcpyPeer
	.globl cudaMemcpyPeerAsync
	.type cudaMemcpyPeerAsync, @function
cudaMemcpyPeerAsync:
	jmp musaMemcpyPeerAsync@PLT
	.size cudaMemcpyPeerAsync, .-cudaMemcpyPeerAsync
	.globl cudaMemcpyToArray
	.type cudaMemcpyToArray, @function
cudaMemcpyToArray:
	jmp musaMemcpyToArray@PLT
	.size cudaMemcpyToArray, .-cudaMemcpyToArray
	.globl cudaMemcpyToArrayAsync
	.type cudaMemcpyToArrayAsync, @function
cudaMemcpyToArrayAsync:
	jmp musaMemcpyToArrayAsync@PLT
	.size cudaMemcpyToArrayAsync, .-cudaMemcpyToArrayAsync
	.globl cudaMemcpyToSymbol
	.type cudaMemcpyToSymbol, @function
cudaMemcpyToSymbol:
	jmp musaMemcpyToSymbol@PLT
	.size cudaMemcpyToSymbol, .-cudaMemcpyToSymbol
	.globl cudaMemcpyToSymbolAsync
	.type cudaMemcpyToSymbolAsync, @function
cudaMemcpyToSymbolAsync:
	jmp musaMemcpyToSymbolAsync@PLT
	.size cudaMemcpyToSymbolAsync, .-cudaMemcpyToSymbolAsync
	.globl cudaMemset
	.type cudaMemset, @function
cudaMemset:
	jmp musaMemset@PLT
	.size cudaMemset, .-cudaMemset
	.globl cudaMemset2D
	.type cudaMemset2D, @function
cudaMemset2D:
	jmp musaMemset2D@PLT
	.size cudaMemset2D, .-cudaMemset2D
	.globl cudaMemset2DAsync
	.type cudaMemset2DAsync, @function
cudaMemset2DAsync:
	jmp musaMemset2DAsync@PLT
	.size cudaMemset2DAsync, .-cudaMemset2DAsync
	.globl cudaMemset3D
	.type cudaMemset3D, @function
cudaMemset3D:
	jmp musaMemset3D@PLT
	.size cudaMemset3D, .-cudaMemset3D
	.globl cudaMemset3DAsync
	.type cudaMemset3DAsync, @function
cudaMemset3DAsync:
	jmp musaMemset3DAsync@PLT
	.size cudaMemset3DAsync, .-cudaMemset3DAsync
	.globl cudaMemsetAsync
	.type cudaMemsetAsync, @function
cudaMemsetAsync:
	jmp musaMemsetAsync@PLT
	.size cudaMemsetAsync, .-cudaMemsetAsync
	.globl cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags
	.type cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags, @function
cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags:
	jmp musaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags@PLT
	.size cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags, .-cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags
	.globl cudaPeekAtLastError
	.type cudaPeekAtLastError, @function
cudaPeekAtLastError:
	jmp musaPeekAtLastError@PLT
	.size cudaPeekAtLastError, .-cudaPeekAtLastError
	.globl cudaPointerGetAttributes
	.type cudaPointerGetAttributes, @function
cudaPointerGetAttributes:
	jmp musaPointerGetAttributes@PLT
	.size cudaPointerGetAttributes, .-cudaPointerGetAttributes
	.globl cudaProfilerStart
	.type cudaProfilerStart, @function
cudaProfilerStart:
	jmp musaProfilerStart@PLT
	.size cudaProfilerStart, .-cudaProfilerStart
	.globl cudaProfilerStop
	.type cudaProfilerStop, @function
cudaProfilerStop:
	jmp musaProfilerStop@PLT
	.size cudaProfilerStop, .-cudaProfilerStop
	.globl cudaRuntimeGetVersion
	.type cudaRuntimeGetVersion, @function
cudaRuntimeGetVersion:
	jmp musaRuntimeGetVersion@PLT
	.size cudaRuntimeGetVersion, .-cudaRuntimeGetVersion
	.globl cudaSetDevice
	.type cudaSetDevice, @function
cudaSetDevice:
	jmp musaSetDevice@PLT
	.size cudaSetDevice, .-cudaSetDevice
	.globl cudaSetDeviceFlags
	.type cudaSetDeviceFlags, @function
cudaSetDeviceFlags:
	jmp musaSetDeviceFlags@PLT
	.size cudaSetDeviceFlags, .-cudaSetDeviceFlags
	.globl cudaStreamAddCallback
	.type cudaStreamAddCallback, @function
cudaStreamAddCallback:
	jmp musaStreamAddCallback@PLT
	.size cudaStreamAddCallback, .-cudaStreamAddCallback
	.globl cudaStreamAttachMemAsync
	.type cudaStreamAttachMemAsync, @function
cudaStreamAttachMemAsync:
	jmp musaStreamAttachMemAsync@PLT
	.size cudaStreamAttachMemAsync, .-cudaStreamAttachMemAsync
	.globl cudaStreamBeginCapture
	.type cudaStreamBeginCapture, @function
cudaStreamBeginCapture:
	jmp musaStreamBeginCapture@PLT
	.size cudaStreamBeginCapture, .-cudaStreamBeginCapture
	.globl cudaStreamCopyAttributes
	.type cudaStreamCopyAttributes, @function
cudaStreamCopyAttributes:
	jmp musaStreamCopyAttributes@PLT
	.size cudaStreamCopyAttributes, .-cudaStreamCopyAttributes
	.globl cudaStreamCreate
	.type cudaStreamCreate, @function
cudaStreamCreate:
	jmp musaStreamCreate@PLT
	.size cudaStreamCreate, .-cudaStreamCreate
	.globl cudaStreamCreateWithFlags
	.type cudaStreamCreateWithFlags, @function
cudaStreamCreateWithFlags:
	jmp musaStreamCreateWithFlags@PLT
	.size cudaStreamCreateWithFlags, .-cudaStreamCreateWithFlags
	.globl cudaStreamCreateWithPriority
	.type cudaStreamCreateWithPriority, @function
cudaStreamCreateWithPriority:
	jmp musaStreamCreateWithPriority@PLT
	.size cudaStreamCreateWithPriority, .-cudaStreamCreateWithPriority
	.globl cudaStreamDestroy
	.type cudaStreamDestroy, @function
cudaStreamDestroy:
	jmp musaStreamDestroy@PLT
	.size cudaStreamDestroy, .-cudaStreamDestroy
	.globl cudaStreamEndCapture
	.type cudaStreamEndCapture, @function
cudaStreamEndCapture:
	jmp musaStreamEndCapture@PLT
	.size cudaStreamEndCapture, .-cudaStreamEndCapture
	.globl cudaStreamGetAttribute
	.type cudaStreamGetAttribute, @function
cudaStreamGetAttribute:
	jmp musaStreamGetAttribute@PLT
	.size cudaStreamGetAttribute, .-cudaStreamGetAttribute
	.globl cudaStreamGetFlags
	.type cudaStreamGetFlags, @function
cudaStreamGetFlags:
	jmp musaStreamGetFlags@PLT
	.size cudaStreamGetFlags, .-cudaStreamGetFlags
	.globl cudaStreamGetPriority
	.type cudaStreamGetPriority, @function
cudaStreamGetPriority:
	jmp musaStreamGetPriority@PLT
	.size cudaStreamGetPriority, .-cudaStreamGetPriority
	.globl cudaStreamQuery
	.type cudaStreamQuery, @function
cudaStreamQuery:
	jmp musaStreamQuery@PLT
	.size cudaStreamQuery, .-cudaStreamQuery
	.globl cudaStreamSetAttribute
	.type cudaStreamSetAttribute, @function
cudaStreamSetAttribute:
	jmp musaStreamSetAttribute@PLT
	.size cudaStreamSetAttribute, .-cudaStreamSetAttribute
	.globl cudaStreamSynchronize
	.type cudaStreamSynchronize, @function
cudaStreamSynchronize:
	jmp musaStreamSynchronize@PLT
	.size cudaStreamSynchronize, .-cudaStreamSynchronize
	.globl cudaStreamWaitEvent
	.type cudaStreamWaitEvent, @function
cudaStreamWaitEvent:
	jmp musaStreamWaitEvent@PLT
	.size cudaStreamWaitEvent, .-cudaStreamWaitEvent
	.globl cudaThreadExit
	.type cudaThreadExit, @function
cudaThreadExit:
	jmp musaThreadExit@PLT
	.size cudaThreadExit, .-cudaThreadExit
	.globl cudaThreadGetCacheConfig
	.type cudaThreadGetCacheConfig, @function
cudaThreadGetCacheConfig:
	jmp musaThreadGetCacheConfig@PLT
	.size cudaThreadGetCacheConfig, .-cudaThreadGetCacheConfig
	.globl cudaThreadGetLimit
	.type cudaThreadGetLimit, @function
cudaThreadGetLimit:
	jmp musaThreadGetLimit@PLT
	.size cudaThreadGetLimit, .-cudaThreadGetLimit
	.globl cudaThreadSetCacheConfig
	.type cudaThreadSetCacheConfig, @function
cudaThreadSetCacheConfig:
	jmp musaThreadSetCacheConfig@PLT
	.size cudaThreadSetCacheConfig, .-cudaThreadSetCacheConfig
	.globl cudaThreadSetLimit
	.type cudaThreadSetLimit, @function
cudaThreadSetLimit:
	jmp musaThreadSetLimit@PLT
	.size cudaThreadSetLimit, .-cudaThreadSetLimit
	.globl cudaThreadSynchronize
	.type cudaThreadSynchronize, @function
cudaThreadSynchronize:
	jmp musaThreadSynchronize@PLT
	.size cudaThreadSynchronize, .-cudaThreadSynchronize
	.globl cudaUnbindTexture
	.type cudaUnbindTexture, @function
cudaUnbindTexture:
	jmp musaUnbindTexture@PLT
	.size cudaUnbindTexture, .-cudaUnbindTexture
	.section .note.GNU-stack,"",@progbits
