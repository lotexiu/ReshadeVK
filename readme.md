# Guia de Desenvolvimento C++ no Fedora KDE

Este repositório contém as configurações básicas para programar em C++ usando ferramentas modernas no Fedora.

## 🛠️ Ferramentas Necessárias (Fedora)

No terminal, instale o grupo de desenvolvimento e os sistemas de build:

```bash
# Build
sudo dnf install meson ninja-build cmake -y
# Dependencias de sistema
sudo dnf install gcc-c++ glfw-devel vulkan-devel vulkan-tools glslang -y
```

## 🧩 Extensões Recomendadas (VS Code)

Para uma experiência completa, instale as seguintes extensões:
1. **C/C++ (Microsoft)**: Provê o IntelliSense e Debugging.
2. **Meson (mesonbuild)**: Suporte ao arquivo \`meson.build\`.

## 🚀 Como Compilar e Executar

1. **Configurar o ambiente (uma única vez):**
   ```bash
   meson setup builddir
   ```

2. **Compilar o código:**
   ```bash
   meson compile -C builddir
   ```

3. **Executar o programa:**
   ```bash
   ./builddir/meu_app
   ```

---
*Dica: Como você está no KDE, a IDE **KDevelop** é uma excelente alternativa ao VS Code.*


```
auto - deduz automaticamente o tipo da variavel

auto x = y; - Cópia (Cria um clone. Se o original for "incopiável", dá erro).

auto& x = y; - Referência (Cria um apelido direto. Não ocupa espaço extra).

auto* x = &y; - Ponteiro (Cria uma variável que guarda o endereço da outra).
```