from pathlib import Path


ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / "HW1_sources.txt"
EXCLUDED_DIRS = {".git", "build", "build_asan", "build_clean", "build_review", "__pycache__"}
EXCLUDED_FILES = {OUTPUT.name, "benchmark.png"}


def main():
    files = []
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file() or path.name in EXCLUDED_FILES:
            continue
        if any(part in EXCLUDED_DIRS for part in path.relative_to(ROOT).parts):
            continue
        try:
            contents = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        files.append((path.relative_to(ROOT).as_posix(), contents))

    with OUTPUT.open("w", encoding="utf-8") as report:
        for relative_path, contents in files:
            report.write(f"===== {relative_path} =====\n")
            report.write(contents)
            if not contents.endswith("\n"):
                report.write("\n")
            report.write("\n")

    print(f"Wrote {len(files)} text files to {OUTPUT.name}")


if __name__ == "__main__":
    main()
