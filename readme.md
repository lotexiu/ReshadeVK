# ReshadeVK

Vulkan overlay layer com ImGui — funciona em X11 e Wayland, com captura de teclado e mouse.

---

## Visão geral da arquitetura

```
src/
  reshadeVK/          ← layer concreta; só define o que desenhar
  VKWindow/           ← janela Vulkan local para desenvolvimento e testes
  utils/              ← framework; toda a complexidade fica aqui, escondida
    layer.hpp/cpp     ← API pública: LayerComponent, RenderContext
    api/vulkan/       ← infraestrutura Vulkan (dispatch, state, renderer, hooks)
    input/            ← providers de input (host bridge, GLFW, X11)
```

### `src/reshadeVK/` — a layer concreta

| Arquivo | Responsabilidade |
|---|---|
| `entry.cpp` | Único arquivo com `extern "C"`. Exporta os dois símbolos que o Vulkan Loader exige. Registra o overlay via `set_layer_component()` no `__attribute__((constructor))`. |
| `overlay.hpp/.cpp` | Define `ReshadeOverlay : LayerComponent`. `onRender()` só contém chamadas ImGui — nenhum Vulkan explícito. |
| `layer.json.in` | Manifesto lido pelo Vulkan Loader. `@library_path@` é substituído pelo Meson em tempo de configure. |

### `src/VKWindow/` — janela Vulkan de desenvolvimento

Processo Vulkan standalone que cria uma janela GLFW e roda a layer em cima, exatamente como um jogo faria.
Permite iterar sem precisar injetar num processo externo.

| Arquivo | Responsabilidade |
|---|---|
| `main.cpp` | Loop principal GLFW. Exporta `reshadevk_host_get_input_state()` para o host bridge. |
| `app.hpp/.cpp` | Encapsula init/destroy/render Vulkan (instance → device → swapchain → render). |
| `vulkan/*/` | Módulos internos: instance, device, swapchain, render. |

### `src/utils/` — o framework

**`layer.hpp`** — API pública. Um desenvolvedor de layer só precisa disto:

```cpp
struct RenderContext {
    VkCommandBuffer cmd;
    VkExtent2D      extent;
    uint32_t        imageIndex;
};

struct LayerComponent {
    virtual void onInit()   {}
    virtual void onRender(const RenderContext& ctx) = 0;
    virtual void onDestroy() {}
};
```

**`api/vulkan/`** — infraestrutura Vulkan interna:

| Arquivo | Responsabilidade |
|---|---|
| `dispatch.hpp/.cpp` | Tabelas de ponteiros PFN via X-Macro. `fill_instance_dispatch` / `fill_device_dispatch`. |
| `state.hpp/.cpp` | `InstanceData`, `DeviceData`, `SwapchainData` e os mapas globais com mutex. |
| `imgui_loader.hpp/.cpp` | Callback `imgui_loader` para `ImGui_ImplVulkan_LoadFunctions`. Contorna o fato de a layer interceptar `vkGetDeviceProcAddr`. |
| `renderer.hpp/.cpp` | Monta recursos de frame (render pass, framebuffers, cmd buffers, semáforos, fences) e chama `LayerComponent::onRender()`. |
| `hooks/instance.cpp` | `vkCreateInstance` / `vkDestroyInstance`. |
| `hooks/physicalDevice.cpp` | `vkEnumeratePhysicalDevices`. |
| `hooks/device.cpp` | `vkCreateDevice` / `vkDestroyDevice`. |
| `hooks/surface.cpp` | `vkDestroySurfaceKHR` + interceptação de `glfwCreateWindowSurface`. |
| `hooks/swapchain.cpp` | `vkCreateSwapchainKHR` → `renderer::init`, `vkQueuePresentKHR` → `renderer::render_frame`. |

**`input/`** — providers de input:

| Arquivo | Responsabilidade |
|---|---|
| `host_bridge.hpp` | Struct `ReshadeVKHostInputState` + `extern "C"` para comunicação com o processo host. |
| `glfw.hpp/.cpp` | Lê input do `GLFWwindow*` associado à swapchain. Usado pelo VKWindow. |
| `x11.hpp/.cpp` | Detecta janela ativa via `_NET_ACTIVE_WINDOW`, confirma PID e lê teclado com `XQueryKeymap`. Usado em jogos reais sob X11. |

**Ordem de prioridade de input por frame:**
1. Host bridge (`reshadevk_host_get_input_state` via dlsym — VKWindow usa este)
2. GLFW (se `glfwWindow` associado à swapchain)
3. X11 (fallback para processos sem GLFW)

**Toggle de interação:** `F8` ativa/desativa a captura de mouse e teclado em runtime.

---

## Como adicionar conteúdo ao overlay

Edite `src/reshadeVK/overlay.cpp`:

```cpp
void ReshadeOverlay::onRender(const RenderContext& ctx) {
    ImGui::Begin("reshadeVK");
    ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);
    // ... seus widgets aqui
    ImGui::End();
}
```

`entry.cpp` já registra `new ReshadeOverlay()` no constructor da SO — nada mais a fazer.

---

## Dependências

```bash
# Fedora / RHEL
sudo dnf install meson ninja-build gcc-c++ \
    glfw-devel vulkan-devel vulkan-tools \
    vulkan-validation-layers glslang libX11-devel -y
```

---

## Build

```bash
# Configurar (uma única vez)
meson setup builddir

# Compilar
meson compile -C builddir

# Executar a janela de desenvolvimento (com a layer ativa)
./builddir/VKWindow
```

A shared library `libreshadevk.so` é gerada em `builddir/`.

---

## Instalar a layer

```bash
meson install -C builddir --destdir ~/.local
```

O manifesto é instalado em `~/.local/share/vulkan/implicit_layer.d/reshadevk.json`.
A layer é carregada automaticamente em qualquer aplicação Vulkan.

Variáveis de controle em runtime:

| Variável | Efeito |
|---|---|
| `ENABLE_RESHADEVK=1` | Força a ativação da layer |
| `DISABLE_RESHADEVK=1` | Desativa a layer |
| `RESHADEVK_INTERACTIVE=0` | Desativa captura de input |

---

## Extensões recomendadas (VS Code)

- **C/C++ (Microsoft)** — IntelliSense e debugging
- **Meson (mesonbuild)** — suporte ao `meson.build`
