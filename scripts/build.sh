#!/usr/bin/env bash
set -euo pipefail

clear

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILDDIR="$PROJECT_ROOT/builddir"

run_step() {
	local message="$1"
	shift
	echo "$message"
	"$@"
}

cd "$PROJECT_ROOT"

if [ ! -d "$BUILDDIR" ]; then
	run_step "Atualizando submódulos..." git submodule update --init --recursive
	run_step "Criando ambiente Meson..." meson setup builddir --prefix "$HOME/.local"
	echo "Criação de ambiente concluída!"
else
	echo "Configuração de ambiente já realizada!"
fi

run_step "Compilando projeto..." ninja -C builddir
run_step "Instalando artefatos..." meson install -C builddir

echo "Build e instalação concluídos com sucesso!"
