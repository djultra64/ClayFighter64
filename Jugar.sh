#!/usr/bin/env bash
set -euo pipefail
clay_project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$clay_project_dir/tools/run.py" "$@"
