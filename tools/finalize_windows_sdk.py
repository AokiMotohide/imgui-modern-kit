#!/usr/bin/env python3
"""Preserve Unicode filenames when archiving the Windows CPack staging tree.

Some Windows libarchive/CPack environments replace Japanese names with '?'.
Re-archive the installed CPack tree using Python's UTF-8 ZIP filename support.
No build, dependency download or changes to the SDK file contents are made.
"""
import argparse
from pathlib import Path
import zipfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    archives = list(build.glob('imgui-modern-kit-*.zip'))
    if len(archives) != 1:
        raise RuntimeError('Expected one CPack SDK ZIP')
    destination = archives[0]
    stages = [path.parent for path in (build / '_CPack_Packages').rglob('README.ja.md')
              if path.parent.name == destination.stem]
    if len(stages) != 1:
        raise RuntimeError('Missing or ambiguous CPack SDK staging tree')
    stage = stages[0]
    required = ['docs/目次.md', 'docs/reference/3.2追加API.md', 'docs/components/トースト.md']
    if any(not (stage / name).is_file() for name in required):
        raise RuntimeError('CPack staging tree is missing Japanese documentation')
    temporary = destination.with_suffix('.utf8.zip')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(stage.rglob('*')):
            if path.is_symlink():
                raise RuntimeError('Unexpected symlink in Windows SDK')
            if path.is_file():
                archive.write(path, destination.stem + '/' + path.relative_to(stage).as_posix())
    with zipfile.ZipFile(temporary) as archive:
        names = set(archive.namelist())
        assert all(destination.stem + '/' + name in names for name in required)
        assert not any('?' in name for name in names)
    temporary.replace(destination)
    print(f'UTF-8 SDK ZIP: {destination.name}; {len(names)} entries')


if __name__ == '__main__':
    main()
