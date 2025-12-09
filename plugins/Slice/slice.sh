#!/usr/bin/env bash
# slice.sh (без любых DOT-генераций)
# $1 — /mnt/c/... путь к исходному .c
# $2 — критерий среза ("main:21:sum" или "21:sum")
set -Eeuo pipefail

export LANG=C.UTF-8
export LC_ALL=C.UTF-8

SRC="$1"
CRIT="$2"

# Временная директория, чтобы не мусорить в /tmp
TMP="$(mktemp -d /tmp/slice.XXXXXX)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

INPUT_BC="$TMP/input.bc"
SLICE_BC="$TMP/slice.bc"
VISIBLE_TMP="$TMP/slice_visible.c"
LLVMC_TMP="$TMP/slice_llvm2c.c"

# ==== 1) .c -> .bc (с дебаг-метой) ====
clang -g -O0 -emit-llvm -c "$SRC" -o "$INPUT_BC"

# ==== 2) Срез ====
SLICER="/mnt/c/Diplom_project/Static_analyser/symbiotic/sbt-slicer/build-10.0.0/src/sbt-slicer"

set +e
"$SLICER" -c "$CRIT" "$INPUT_BC" -o "$SLICE_BC"
SLICER_RC=$?
set -e

# Если слайсер вернул не-0, но slice.bc существует — считаем частичным успехом
if [[ $SLICER_RC -ne 0 ]]; then
  if [[ -s "$SLICE_BC" ]]; then
    # Помечаем статус в комменте — на случай, если IDE захочет подсветить предупреждение
    echo "/*__SLICER_STATUS__ exit_code:$SLICER_RC (nonzero but slice.bc exists) */"
  else
    echo "sbt-slicer failed with code $SLICER_RC" >&2
    exit "$SLICER_RC"
  fi
fi

# ==== 2.5) Генерация CFG .dot ====
# ВАЖНО: запускаем opt внутри $TMP, чтобы .dot-файлы оказались там же
pushd "$TMP" >/dev/null
# Не даём ошибкам убить скрипт (из-за set -e/pipefail):
opt -dot-cfg "$SLICE_BC" -disable-output -o /dev/null 2>/dev/null || true
popd >/dev/null

# Берём .dot для main (или первый попавшийся)
CFG_DOT="$(ls "$TMP"/*.dot 2>/dev/null | grep '\.main\.' | head -n1 || true)"
if [[ -z "$CFG_DOT" ]]; then
  CFG_DOT="$(ls "$TMP"/*.dot 2>/dev/null | head -n1 || true)"
fi

emit_dot_cfg() {
  if [[ -f "$CFG_DOT" ]]; then
    {
      echo 'digraph CFG { rankdir=LR; node [shape=box,fontname="Courier"];'
      # выкидываем первую и последнюю строки исходного .dot (они уже будут в нашем обёрточном графе)
      sed '1d;$d' "$CFG_DOT"
      echo '}'
    }
  else
    echo 'digraph CFG { label="no CFG produced"; }'
  fi
}
DOT_CFG="$(emit_dot_cfg)"


# ==== 3) Собираем список уникальных исходных строк из slice.bc (по !DILocation) ====
mapfile -t LINES < <(
  llvm-dis -o - "$SLICE_BC" \
  | grep -oP '!DILocation\(\s*line:\s*\K\d+' \
  | sort -n | uniq
)

mapfile -t FUN_LINES < <(
  llvm-dis -o - "$SLICE_BC" \
  | tr -d '\r' \
  | grep -oP '!DISubprogram\([^)]*\)' \
  | grep -oP '(?:line|scopeLine):\s*\K\d+' \
  | sort -n | uniq
)

if ((${#FUN_LINES[@]} > 0)); then
  LINES+=("${FUN_LINES[@]}")
  mapfile -t LINES < <(printf '%s\n' "${LINES[@]}" | sort -n | uniq)
fi

# ==== 4) Формируем «видимые» строки и JSON-карту ====
: > "$VISIBLE_TMP"
SRC_BASENAME="$(basename -- "$SRC")"

LINE_MAP_JSON='{"lineMap":{'
idx=1
for ln in "${LINES[@]}"; do
  # строка исходника
  sed -n "${ln}p" "$SRC" >> "$VISIBLE_TMP"
  # элемент карты
  LINE_MAP_JSON+=$(printf '"%d":{"file":"%s","line":%d},' "$idx" "$SRC_BASENAME" "$ln")
  idx=$((idx+1))
done
LINE_MAP_JSON="${LINE_MAP_JSON%,}"
LINE_MAP_JSON+='}}'

# ==== 5) Полная декомпиляция llvm2c (оставляем, это не DOT) ====
LLVMC="/mnt/c/Diplom_project/Static_analyser/symbiotic/llvm2c/build-10.0.0/llvm2c"
if ! "$LLVMC" "$SLICE_BC" -o "$LLVMC_TMP"; then
  : > "$LLVMC_TMP"   # если llvm2c упал — отдаём пустой блок, но не ломаемся
fi

emit_dot_visible() {
  echo 'digraph G {'
  echo '  node [shape=box, fontname="Courier"];'
  awk '{
    gsub(/"/,"\\\"");
    printf "  L%d [label=\"%s\"];\n", NR, $0;
    if (NR>1) printf "  L%d -> L%d;\n", NR-1, NR;
  }' "$VISIBLE_TMP"
  echo '}'
}
DOT_VISIBLE="$(emit_dot_visible)"


# ==== 6) Отдаём бандл в stdout ====
printf "/*__BEGIN_ORIGINAL_CUSTOM__*/"
cat "$VISIBLE_TMP"
echo "/*__END_ORIGINAL_CUSTOM__*/"

printf "/*__BEGIN_LLVM2C__*/"
cat "$LLVMC_TMP"
echo "/*__END_LLVM2C__*/"

printf "/*__IDE_METADATA__\n%s\n__IDE_METADATA_END__*/\n" "$LINE_MAP_JSON"

printf "/*__BEGIN_DOT_VISIBLE__*/\n%s\n/*__END_DOT_VISIBLE__*/\n" "$DOT_VISIBLE"
printf "/*__BEGIN_DOT_CFG__*/\n%s\n/*__END_DOT_CFG__*/\n" "$DOT_CFG"





