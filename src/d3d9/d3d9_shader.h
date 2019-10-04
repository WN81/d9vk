#pragma once

#include "d3d9_resource.h"
#include "../dxso/dxso_module.h"
#include "d3d9_shade_mode_emu.h"

namespace dxvk {

  /**
   * \brief Common shader object
   * 
   * Stores the compiled SPIR-V shader and the SHA-1
   * hash of the original DXBC shader, which can be
   * used to identify the shader.
   */
  class D3D9CommonShader {

  public:

    D3D9CommonShader();

    D3D9CommonShader(
            D3D9DeviceEx*         pDevice,
      const DxvkShaderKey*        pShaderKey,
      const DxsoModuleInfo*       pDxbcModuleInfo,
      const void*                 pShaderBytecode,
      const DxsoAnalysisInfo&     AnalysisInfo,
            DxsoModule*           pModule);


    Rc<DxvkShader> GetShader() const {
      return m_shader;
    }

    std::string GetName() const {
      return m_shader->debugName();
    }

    const std::vector<uint8_t>& GetBytecode() const {
      return m_bytecode;
    }

    const DxsoIsgn& GetIsgn() const {
      return m_isgn;
    }

    D3D9ShadeModeElements GetShadeElements() const {
      return m_shadeElements;
    }

    const DxsoShaderMetaInfo& GetMeta() const { return m_meta; }
    const DxsoDefinedConstants& GetConstants() const { return m_constants; }
    bool IsSamplerUsed(uint32_t index) const {
      return m_usedSamplers & (1u << index);
    }

    bool IsRTUsed(uint32_t index) const {
      return m_usedRTs & (1u << index);
    }

    const DxsoProgramInfo& GetInfo() const { return m_info; }

  private:

    DxsoIsgn              m_isgn;
    uint32_t              m_usedSamplers;
    uint32_t              m_usedRTs;

    D3D9ShadeModeElements m_shadeElements = { };

    DxsoProgramInfo       m_info;
    DxsoShaderMetaInfo    m_meta;
    DxsoDefinedConstants  m_constants;

    Rc<DxvkShader>        m_shader;

    std::vector<uint8_t>  m_bytecode;

  };

  /**
   * \brief Common shader interface
   * 
   * Implements methods for all D3D11*Shader
   * interfaces and stores the actual shader
   * module object.
   */
  template <typename Base>
  class D3D9Shader : public D3D9DeviceChild<Base> {

  public:

    D3D9Shader(
            D3D9DeviceEx*      pDevice,
      const D3D9CommonShader&  CommonShader)
      : D3D9DeviceChild<Base>( pDevice )
      , m_shader             ( CommonShader ) { }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
      if (ppvObject == nullptr)
        return E_POINTER;

      *ppvObject = nullptr;

      if (riid == __uuidof(IUnknown)
       || riid == __uuidof(Base)) {
        *ppvObject = ref(this);
        return S_OK;
      }

      Logger::warn("D3D9Shader::QueryInterface: Unknown interface query");
      Logger::warn(str::format(riid));
      return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetFunction(void* pOut, UINT* pSizeOfData) {
      if (pSizeOfData == nullptr)
        return D3DERR_INVALIDCALL;

      const auto& bytecode = m_shader.GetBytecode();

      if (pOut == nullptr) {
        *pSizeOfData = bytecode.size();
        return D3D_OK;
      }

      size_t copyAmount = std::min(size_t(*pSizeOfData), bytecode.size());
      std::memcpy(pOut, bytecode.data(), copyAmount);

      return D3D_OK;
    }

    const D3D9CommonShader* GetCommonShader() const {
      return &m_shader;
    }

  private:

    D3D9CommonShader m_shader;

  };

  // Needs their own classes and not usings for forward decl.

  class D3D9VertexShader final : public D3D9Shader<IDirect3DVertexShader9> {

  public:

    D3D9VertexShader(
            D3D9DeviceEx*      pDevice,
      const D3D9CommonShader&  CommonShader)
      : D3D9Shader<IDirect3DVertexShader9>( pDevice, CommonShader ) { }

  };

  class D3D9PixelShader final : public D3D9Shader<IDirect3DPixelShader9> {

  public:

    D3D9PixelShader(
            D3D9DeviceEx*      pDevice,
      const D3D9CommonShader&  CommonShader)
      : D3D9Shader<IDirect3DPixelShader9>( pDevice, CommonShader ) { }

  };

  /**
   * \brief Shader module set
   * 
   * Some applications may compile the same shader multiple
   * times, so we should cache the resulting shader modules
   * and reuse them rather than creating new ones. This
   * class is thread-safe.
   */
  class D3D9ShaderModuleSet : public RcObject {
    
  public:
    
    D3D9CommonShader GetShaderModule(
            D3D9DeviceEx*         pDevice,
            VkShaderStageFlagBits ShaderStage,
      const DxsoModuleInfo*       pDxbcModuleInfo,
      const void*                 pShaderBytecode);
    
  private:
    
    std::mutex m_mutex;
    
    std::unordered_map<
      DxvkShaderKey,
      D3D9CommonShader,
      DxvkHash, DxvkEq> m_modules;
    
};

}