#pragma once

#include "../dxvk/dxvk_include.h"

#include "dxgi_format.h"
#include "dxgi_include.h"

namespace dxvk {
  class DxgiAdapter;
  class DxgiSwapChain;
  class DxvkAdapter;
  class DxvkBuffer;
  class DxvkDevice;
  class DxvkImage;
}

struct IDXGIVkInteropDevice;


/**
 * \brief Per-monitor data
 */
struct DXGI_VK_MONITOR_DATA {
  dxvk::DxgiSwapChain*  pSwapChain;
  DXGI_FRAME_STATISTICS FrameStats;
  DXGI_GAMMA_CONTROL    GammaCurve;
};


/**
 * \brief Private DXGI presenter
 * 
 * Presenter interface that allows the DXGI swap
 * chain implementation to remain API-agnostic,
 * so that common code can stay in one class.
 */
MIDL_INTERFACE("104001a6-7f36-4957-b932-86ade9567d91")
IDXGIVkSwapChain : public IUnknown {
  static const GUID guid;

  virtual HRESULT STDMETHODCALLTYPE GetDesc(
          DXGI_SWAP_CHAIN_DESC1*    pDesc) = 0;
  
  virtual HRESULT STDMETHODCALLTYPE GetAdapter(
          REFIID                    riid,
          void**                    ppvObject) = 0;
  
  virtual HRESULT STDMETHODCALLTYPE GetDevice(
          REFIID                    riid,
          void**                    ppDevice) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetImage(
          UINT                      BufferId,
          REFIID                    riid,
          void**                    ppBuffer) = 0;

  virtual UINT STDMETHODCALLTYPE GetImageIndex() = 0;

  virtual UINT STDMETHODCALLTYPE GetFrameLatency() = 0;

  virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyEvent() = 0;

  virtual HRESULT STDMETHODCALLTYPE ChangeProperties(
    const DXGI_SWAP_CHAIN_DESC1*    pDesc) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetPresentRegion(
    const RECT*                     pRegion) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetGammaControl(
          UINT                      NumControlPoints,
    const DXGI_RGB*                 pControlPoints) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetFrameLatency(
          UINT                      MaxLatency) = 0;

  virtual HRESULT STDMETHODCALLTYPE Present(
          UINT                      SyncInterval,
          UINT                      PresentFlags,
    const DXGI_PRESENT_PARAMETERS*  pPresentParameters) = 0;
};


/**
 * \brief Private DXGI adapter interface
 * 
 * The implementation of \c IDXGIAdapter holds a
 * \ref DxvkAdapter which can be retrieved using
 * this interface.
 */
MIDL_INTERFACE("907bf281-ea3c-43b4-a8e4-9f231107b4ff")
IDXGIDXVKAdapter : public IDXGIAdapter3 {
  static const GUID guid;
  
  virtual dxvk::Rc<dxvk::DxvkAdapter> STDMETHODCALLTYPE GetDXVKAdapter() = 0;

  virtual dxvk::Rc<dxvk::DxvkInstance> STDMETHODCALLTYPE GetDXVKInstance() = 0;

};


/**
 * \brief Private DXGI monitor info interface
 * 
 * Can be queried from the DXGI factory to store monitor
 * info globally, with a lifetime that exceeds that of
 * the \c IDXGIOutput or \c IDXGIAdapter objects.
 */
MIDL_INTERFACE("c06a236f-5be3-448a-8943-89c611c0c2c1")
IDXGIVkMonitorInfo : public IUnknown {
  static const GUID guid;

  /**
   * \brief Initializes monitor data
   * 
   * Fails if data for the given monitor already exists.
   * \param [in] hMonitor The monitor handle
   * \param [in] pData Initial data
   */
  virtual HRESULT STDMETHODCALLTYPE InitMonitorData(
          HMONITOR                hMonitor,
    const DXGI_VK_MONITOR_DATA*   pData) = 0;

  /**
   * \brief Retrieves and locks monitor data
   * 
   * Fails if no data for the given monitor exists.
   * \param [in] hMonitor The monitor handle
   * \param [out] Pointer to monitor data
   * \returns S_OK on success
   */
  virtual HRESULT STDMETHODCALLTYPE AcquireMonitorData(
          HMONITOR                hMonitor,
          DXGI_VK_MONITOR_DATA**  ppData) = 0;
  
  /**
   * \brief Unlocks monitor data
   * 
   * Must be called after each successful
   * call to \ref AcquireMonitorData.
   * \param [in] hMonitor The monitor handle
   */
  virtual void STDMETHODCALLTYPE ReleaseMonitorData() = 0;

};


/**
 * \brief DXGI surface interface for Vulkan interop
 * 
 * Provides access to the backing resource of a
 * DXGI surface, which is typically a D3D texture.
 */
MIDL_INTERFACE("5546cf8c-77e7-4341-b05d-8d4d5000e77d")
IDXGIVkInteropSurface : public IUnknown {
  static const GUID guid;
  
  /**
   * \brief Retrieves device interop interfaceSlots
   * 
   * Queries the device that owns the surface for
   * the \ref IDXGIVkInteropDevice interface.
   * \param [out] ppDevice The device interface
   * \returns \c S_OK on success
   */
  virtual HRESULT STDMETHODCALLTYPE GetDevice(
          IDXGIVkInteropDevice**  ppDevice) = 0;
  
  /**
   * \brief Retrieves Vulkan image info
   * 
   * Retrieves both the image handle as well as the image's
   * properties. Any of the given pointers may be \c nullptr.
   * 
   * If \c pInfo is not \c nullptr, the following rules apply:
   * - \c pInfo->sType \e must be \c VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
   * - \c pInfo->pNext \e must be \c nullptr or point to a supported
   *   extension-specific structure (currently none)
   * - \c pInfo->queueFamilyIndexCount must be the length of the
   *   \c pInfo->pQueueFamilyIndices array, in \c uint32_t units.
   * - \c pInfo->pQueueFamilyIndices must point to a pre-allocated
   *   array of \c uint32_t of size \c pInfo->pQueueFamilyIndices.
   * 
   * \note As of now, the sharing mode will always be
   *       \c VK_SHARING_MODE_EXCLUSIVE and no queue
   *       family indices will be written to the array.
   * 
   * After the call, the structure pointed to by \c pInfo can
   * be used to create an image with identical properties.
   * 
   * If \c pLayout is not \c nullptr, it will receive the
   * layout that the image will be in after flushing any
   * outstanding commands on the device.
   * \param [out] pHandle The image handle
   * \param [out] pLayout Image layout
   * \param [out] pInfo Image properties
   * \returns \c S_OK on success, or \c E_INVALIDARG
   */
  virtual HRESULT STDMETHODCALLTYPE GetVulkanImageInfo(
          VkImage*              pHandle,
          VkImageLayout*        pLayout,
          VkImageCreateInfo*    pInfo) = 0;
};


/**
 * \brief DXGI device interface for Vulkan interop
 * 
 * Provides access to the device and instance handles
 * as well as the queue that is used for rendering.
 */
MIDL_INTERFACE("e2ef5fa5-dc21-4af7-90c4-f67ef6a09323")
IDXGIVkInteropDevice : public IUnknown {
  static const GUID guid;
  
  /**
   * \brief Queries Vulkan handles used by DXVK
   * 
   * \param [out] pInstance The Vulkan instance
   * \param [out] pPhysDev The physical device
   * \param [out] pDevide The device handle
   */
  virtual void STDMETHODCALLTYPE GetVulkanHandles(
          VkInstance*           pInstance,
          VkPhysicalDevice*     pPhysDev,
          VkDevice*             pDevice) = 0;
  
  /**
   * \brief Queries the rendering queue used by DXVK
   * 
   * \param [out] pQueue The Vulkan queue handle
   * \param [out] pQueueFamilyIndex Queue family index
   */
  virtual void STDMETHODCALLTYPE GetSubmissionQueue(
          VkQueue*              pQueue,
          uint32_t*             pQueueFamilyIndex) = 0;
  
  /**
   * \brief Transitions a surface to a given layout
   * 
   * Executes an explicit image layout transition on the
   * D3D device. Note that the image subresources \e must
   * be transitioned back to its original layout before
   * using it again from D3D11.
   * \param [in] pSurface The image to transform
   * \param [in] pSubresources Subresources to transform
   * \param [in] OldLayout Current image layout
   * \param [in] NewLayout Desired image layout
   */
  virtual void STDMETHODCALLTYPE TransitionSurfaceLayout(
          IDXGIVkInteropSurface*    pSurface,
    const VkImageSubresourceRange*  pSubresources,
          VkImageLayout             OldLayout,
          VkImageLayout             NewLayout) = 0;
  
  /**
   * \brief Flushes outstanding D3D rendering commands
   * 
   * Must be called before submitting Vulkan commands
   * to the rendering queue if those commands use the
   * backing resource of a D3D11 object.
   */
  virtual void STDMETHODCALLTYPE FlushRenderingCommands() = 0;
  
  /**
   * \brief Locks submission queue
   * 
   * Should be called immediately before submitting
   * Vulkan commands to the rendering queue in order
   * to prevent DXVK from using the queue.
   * 
   * While the submission queue is locked, no D3D11
   * methods must be called from the locking thread,
   * or otherwise a deadlock might occur.
   */
  virtual void STDMETHODCALLTYPE LockSubmissionQueue() = 0;
  
  /**
   * \brief Releases submission queue
   * 
   * Should be called immediately after submitting
   * Vulkan commands to the rendering queue in order
   * to allow DXVK to submit new commands.
   */
  virtual void STDMETHODCALLTYPE ReleaseSubmissionQueue() = 0;
};


/**
 * \brief DXGI adapter interface for Vulkan interop
 *
 * Provides access to the physical device and
 * instance handles for the given DXGI adapter.
 */
MIDL_INTERFACE("3a6d8f2c-b0e8-4ab4-b4dc-4fd24891bfa5")
IDXGIVkInteropAdapter : public IUnknown {
  static const GUID guid;

  /**
   * \brief Queries Vulkan handles used by DXVK
   * 
   * \param [out] pInstance The Vulkan instance
   * \param [out] pPhysDev The physical device
   */
  virtual void STDMETHODCALLTYPE GetVulkanHandles(
          VkInstance*           pInstance,
          VkPhysicalDevice*     pPhysDev) = 0;
};


/**
 * \brief IWineDXGISwapChainFactory device interface
 *
 * Allows a swap chain to be created from a device.
 * See include/wine/winedxgi.idl for definition.
 */
MIDL_INTERFACE("53cb4ff0-c25a-4164-a891-0e83db0a7aac")
IWineDXGISwapChainFactory : public IUnknown {
    static const GUID guid;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(
            IDXGIFactory*           pFactory,
            HWND                    hWnd,
      const DXGI_SWAP_CHAIN_DESC1*  pDesc,
      const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
            IDXGIOutput*            pRestrictToOutput,
            IDXGISwapChain1**       ppSwapChain) = 0;
};


#ifdef _MSC_VER
struct __declspec(uuid("907bf281-ea3c-43b4-a8e4-9f231107b4ff")) IDXGIDXVKAdapter;
struct __declspec(uuid("c06a236f-5be3-448a-8943-89c611c0c2c1")) IDXGIVkMonitorInfo;
struct __declspec(uuid("3a6d8f2c-b0e8-4ab4-b4dc-4fd24891bfa5")) IDXGIVkInteropAdapter;
struct __declspec(uuid("e2ef5fa5-dc21-4af7-90c4-f67ef6a09323")) IDXGIVkInteropDevice;
struct __declspec(uuid("5546cf8c-77e7-4341-b05d-8d4d5000e77d")) IDXGIVkInteropSurface;
struct __declspec(uuid("104001a6-7f36-4957-b932-86ade9567d91")) IDXGIVkSwapChain;
struct __declspec(uuid("53cb4ff0-c25a-4164-a891-0e83db0a7aac")) IWineDXGISwapChainFactory;
#else
DXVK_DEFINE_GUID(IDXGIDXVKAdapter);
DXVK_DEFINE_GUID(IDXGIVkMonitorInfo);
DXVK_DEFINE_GUID(IDXGIVkInteropAdapter);
DXVK_DEFINE_GUID(IDXGIVkInteropDevice);
DXVK_DEFINE_GUID(IDXGIVkInteropSurface);
DXVK_DEFINE_GUID(IDXGIVkSwapChain);
DXVK_DEFINE_GUID(IWineDXGISwapChainFactory);
#endif