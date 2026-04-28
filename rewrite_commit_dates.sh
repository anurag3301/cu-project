#!/usr/bin/env bash
set -euo pipefail

# Update these variables as needed.
START_COMMIT="46c27d9a1c6fee8feebd1a33a3553f6e54ffd9b5"
RANGE_INPUT="may1-may21 2026"
ACTUAL_DAYS=10
TIMEZONE_OFFSET="+0530"

usage() {
  cat <<USAGE
Usage:
  $0 [--dry-run]

Config is defined at top of this script:
- START_COMMIT
- RANGE_INPUT
- ACTUAL_DAYS
- TIMEZONE_OFFSET
USAGE
}

DRY_RUN=0
if [[ $# -gt 1 ]]; then
  usage
  exit 1
fi
if [[ $# -eq 1 ]]; then
  case "$1" in
    --dry-run) DRY_RUN=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown arg: $1" >&2; usage; exit 1 ;;
  esac
fi

if ! [[ "$ACTUAL_DAYS" =~ ^[0-9]+$ ]] || (( ACTUAL_DAYS <= 0 )); then
  echo "error: ACTUAL_DAYS must be a positive integer" >&2
  exit 1
fi
if ! [[ "$TIMEZONE_OFFSET" =~ ^[+-][0-9]{4}$ ]]; then
  echo "error: TIMEZONE_OFFSET must look like +0200 or -0500" >&2
  exit 1
fi
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "error: run inside a git repository" >&2
  exit 1
fi
if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  echo "error: tracked working tree changes detected. commit/stash first" >&2
  exit 1
fi
if ! git merge-base --is-ancestor "$START_COMMIT" HEAD; then
  echo "error: START_COMMIT is not an ancestor of HEAD: $START_COMMIT" >&2
  exit 1
fi

month_to_num() {
  case "$(echo "$1" | tr '[:upper:]' '[:lower:]')" in
    jan|january) echo 01 ;;
    feb|february) echo 02 ;;
    mar|march) echo 03 ;;
    apr|april) echo 04 ;;
    may) echo 05 ;;
    jun|june) echo 06 ;;
    jul|july) echo 07 ;;
    aug|august) echo 08 ;;
    sep|sept|september) echo 09 ;;
    oct|october) echo 10 ;;
    nov|november) echo 11 ;;
    dec|december) echo 12 ;;
    *) return 1 ;;
  esac
}

START_DATE=""
END_DATE=""
if [[ "$RANGE_INPUT" =~ ^([A-Za-z]+)([0-9]{1,2})-([A-Za-z]+)([0-9]{1,2})([[:space:]]+([0-9]{4}))?$ ]]; then
  m1="${BASH_REMATCH[1]}"
  d1="${BASH_REMATCH[2]}"
  m2="${BASH_REMATCH[3]}"
  d2="${BASH_REMATCH[4]}"
  year="${BASH_REMATCH[6]:-$(date +%Y)}"

  mm1="$(month_to_num "$m1")" || { echo "error: invalid month in RANGE_INPUT" >&2; exit 1; }
  mm2="$(month_to_num "$m2")" || { echo "error: invalid month in RANGE_INPUT" >&2; exit 1; }

  START_DATE="$(printf "%04d-%02d-%02d" "$year" "$mm1" "$d1")"
  END_DATE="$(printf "%04d-%02d-%02d" "$year" "$mm2" "$d2")"
elif [[ "$RANGE_INPUT" =~ ^([0-9]{4}-[0-9]{2}-[0-9]{2}):([0-9]{4}-[0-9]{2}-[0-9]{2})$ ]]; then
  START_DATE="${BASH_REMATCH[1]}"
  END_DATE="${BASH_REMATCH[2]}"
else
  echo "error: unsupported RANGE_INPUT format. e.g. 'may20-may25 2026' or '2026-05-20:2026-05-25'" >&2
  exit 1
fi

start_epoch="$(date -d "$START_DATE" +%s)"
end_epoch="$(date -d "$END_DATE" +%s)"
if (( start_epoch > end_epoch )); then
  echo "error: range start is after end" >&2
  exit 1
fi

mapfile -t RANGE_DATES < <(
  cur="$START_DATE"
  while :; do
    echo "$cur"
    [[ "$cur" == "$END_DATE" ]] && break
    cur="$(date -d "$cur +1 day" +%F)"
  done
)
if (( ACTUAL_DAYS > ${#RANGE_DATES[@]} )); then
  echo "error: ACTUAL_DAYS ($ACTUAL_DAYS) exceeds days in range (${#RANGE_DATES[@]})" >&2
  exit 1
fi

mapfile -t CHOSEN_DATES < <(printf '%s\n' "${RANGE_DATES[@]}" | shuf | head -n "$ACTUAL_DAYS" | sort)
mapfile -t COMMITS < <(git rev-list --reverse "${START_COMMIT}^..HEAD")

commit_count="${#COMMITS[@]}"
if (( commit_count == 0 )); then
  echo "error: no commits found in ${START_COMMIT}^..HEAD" >&2
  exit 1
fi

base=$(( commit_count / ACTUAL_DAYS ))
rem=$(( commit_count % ACTUAL_DAYS ))

mapping_file=".git/date_rewrite_map.tsv"
chosen_file=".git/date_rewrite_days.txt"
: > "$mapping_file"
printf '%s\n' "${CHOSEN_DATES[@]}" > "$chosen_file"

idx=0
for day_idx in "${!CHOSEN_DATES[@]}"; do
  day="${CHOSEN_DATES[$day_idx]}"
  day_commits=$base
  if (( day_idx < rem )); then
    day_commits=$((day_commits + 1))
  fi

  for ((j=0; j<day_commits; j++)); do
    commit="${COMMITS[$idx]}"
    author_iso="$(git show -s --format=%aI "$commit")"
    hhmmss="${author_iso:11:8}"
    printf "%s\t%s %s %s\n" "$commit" "$day" "$hhmmss" "$TIMEZONE_OFFSET" >> "$mapping_file"
    idx=$((idx + 1))
  done
done

echo "start_commit: $START_COMMIT"
echo "rewrite_count: $commit_count"
echo "range: $START_DATE .. $END_DATE"
echo "timezone: $TIMEZONE_OFFSET"
echo "chosen_days (${#CHOSEN_DATES[@]}): ${CHOSEN_DATES[*]}"

if (( DRY_RUN == 1 )); then
  echo
  echo "dry-run mapping preview:"
  sed -n '1,80p' "$mapping_file"
  exit 0
fi

branch="$(git rev-parse --abbrev-ref HEAD)"
backup_tag="backup-before-date-rewrite-$(date +%Y%m%d-%H%M%S)"
git tag "$backup_tag"
echo "created backup tag: $backup_tag"

export FILTER_BRANCH_SQUELCH_WARNING=1
export MAP_FILE="$(pwd)/$mapping_file"

git filter-branch -f --env-filter '
new_date=$(awk -v c="$GIT_COMMIT" '\''$1==c{print substr($0, index($0, $2)); exit}'\'' "$MAP_FILE")
if [ -n "$new_date" ]; then
  export GIT_AUTHOR_DATE="$new_date"
  export GIT_COMMITTER_DATE="$new_date"
fi
' -- "${START_COMMIT}^..HEAD"

echo "rewrite complete on branch $branch"
echo "mapping file: $mapping_file"
echo "days file: $chosen_file"
echo "backup tag: $backup_tag"
