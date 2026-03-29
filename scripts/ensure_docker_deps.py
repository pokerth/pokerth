#!/usr/bin/env python3
"""
Ensure vcpkg + Qt readiness inside a Linux container, then optionally run `make`.

Invocation (only Docker / devcontainer flows):
  `run_devcontainer.py` passes this as the container command for `make *-docker`;
  devcontainer `postCreateCommand` may run it on first open (Windows).

NOT used for native host setup:
  Use `make setup-<platform>` or `scripts/setup.sh` directly — different BUILD_DIR, ROOT,
  and no `ensure` orchestration.

Docker build vs docker run (contract for supported kinds — see _KIND_DEFAULT_VCPKG_TRIPLET):
  - Toolchain (`setup.sh toolchain`) is expected to live in the **image** from `docker build`,
    not be re-run here except if we add an explicit recovery path.
  - Deps (Qt, vcpkg under the mounted cache) run only under **`docker run`** with a real `-v`
    to the host cache. `ensure` invokes **`setup.sh deps`** when `docker_deps_ready` is false
    (vcpkg and/or Qt) — not the same as a full host **`setup.sh all`**.
  - For kinds in _KIND_DEFAULT_VCPKG_TRIPLET, invoke setup with the **deps** stage only
    (not full **all**); do not assume host Makefile stamp rules inside this entrypoint.

Readiness (`docker_deps_ready`): (1) `vcpkg_deps_ready` — check_port (protobuf) for primary
triplet + _KIND_EXTRA_READINESS_TRIPLETS; (2) `qt_cmake_ready` — executable **qt-cmake**
under QT_OUTPUT_DIR or ROOT/Qt (same for Android and Windows).
"""

from __future__ import annotations

import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


def run_checked(cmd: list[str], env: dict[str, str] | None = None, cwd: Path | None = None) -> None:
    subprocess.run(cmd, check=True, env=env, cwd=str(cwd) if cwd else None)


def touch_stamp_file(stamp_path: Path) -> None:
    """
    Create empty stamp; replace an existing file we cannot write (e.g. root-owned after sudo docker).

    On Unix, the directory owner may unlink a root-owned file in that directory, then recreate it.
    """

    stamp_path = stamp_path.resolve()
    stamp_path.parent.mkdir(parents=True, exist_ok=True)
    if stamp_path.exists() and not os.access(stamp_path, os.W_OK):
        try:
            stamp_path.unlink()
        except OSError as exc:
            raise PermissionError(
                f"Cannot remove stale stamp {stamp_path} ({exc}). "
                f"Fix: sudo chown -R \"$(id -u)\":\"$(id -g)\" {stamp_path.parent}"
            ) from exc
    try:
        stamp_path.touch()
    except OSError as exc:
        raise PermissionError(
            f"Cannot write stamp {stamp_path} ({exc}). "
            f"Fix: sudo chown -R \"$(id -u)\":\"$(id -g)\" {stamp_path.parent}"
        ) from exc


def vcpkg_ready(vcpkg_root: Path, triplet: str, port: str) -> bool:
    vcpkg_bin = vcpkg_root / "vcpkg"
    installed_dir = vcpkg_root / "installed"
    if not vcpkg_bin.exists() or not installed_dir.is_dir():
        return False

    try:
        proc = subprocess.run(
            [str(vcpkg_bin), "list", f"--triplet={triplet}"],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except OSError:
        return False

    for line in (proc.stdout or "").splitlines():
        l = line.strip()
        if l.startswith(f"{port}[") or l.startswith(f"{port}:"):
            # `vcpkg list` can be true even when an interrupted build left the port
            # partially installed (e.g. protobuf present but ProtobufConfig.cmake missing).
            if port == "protobuf":
                installed_root = vcpkg_root / "installed" / triplet
                # Depending on the vcpkg port/version, protobuf's config may live in
                # either `lib/cmake/protobuf/` or `share/protobuf/`.
                candidates = [
                    installed_root / "lib" / "cmake" / "protobuf",
                    installed_root / "share" / "protobuf",
                ]
                for d in candidates:
                    if (d / "ProtobufConfig.cmake").exists() or (d / "protobuf-config.cmake").exists():
                        return True
                return False
            return True
    return False


@dataclass(frozen=True)
class EnsurePlan:
    kind: str
    vcpkg_root: Path
    cache_dir: Path
    triplet: str
    check_port: str
    run_make_target: str | None
    run_make_extra_env: dict[str, str]
    setup_stamp_file: str | None


def target_to_kind(target: str) -> str:
    kind = target.split("-", 1)[0]
    return kind


def kind_run_make_targets(kind: str) -> frozenset[str]:
    """Targets that run `make <target>` after deps setup: <kind> and <kind>-installer."""
    return frozenset((kind, f"{kind}-installer"))


_KIND_DEFAULT_VCPKG_TRIPLET: dict[str, str] = {
    "windows": "x64-mingw-static",
    "android": "arm64-android",
}

# Additional triplets that must pass the same vcpkg_ready(check_port) probe as the primary triplet.
_KIND_EXTRA_READINESS_TRIPLETS: dict[str, frozenset[str]] = {
    "windows": frozenset(),
    "android": frozenset(("x64-linux",)),
}


def vcpkg_deps_ready(plan: EnsurePlan) -> bool:
    """True if check_port is ready for the primary triplet and any kind-specific extra triplets."""
    extra = _KIND_EXTRA_READINESS_TRIPLETS.get(plan.kind, frozenset())
    triplets = frozenset((plan.triplet,)) | extra
    return all(vcpkg_ready(plan.vcpkg_root, t, plan.check_port) for t in triplets)


def qt_install_base(plan: EnsurePlan) -> Path:
    """Root of the aqt Qt tree: QT_OUTPUT_DIR when set, else ROOT/Qt (devcontainer / run_devcontainer)."""
    return Path(os.environ.get("QT_OUTPUT_DIR", str(plan.cache_dir / "Qt")))


def qt_cmake_ready(qt_base: Path) -> bool:
    """True if some **qt-cmake** under qt_base exists and is executable (mingw_64 / gcc_64 / etc.)."""
    if not qt_base.is_dir():
        return False
    try:
        for p in qt_base.rglob("qt-cmake"):
            if p.is_file() and os.access(p, os.X_OK):
                return True
    except OSError:
        return False
    return False


def docker_deps_ready(plan: EnsurePlan) -> bool:
    """vcpkg + Qt (qt-cmake on disk under the mounted Qt tree)."""
    return vcpkg_deps_ready(plan) and qt_cmake_ready(qt_install_base(plan))


def build_ensure_plan(target: str) -> EnsurePlan:
    kind = target_to_kind(target)

    root_default = Path(f"/opt/pokerth-{kind}")
    root = Path(os.environ.get("ROOT", str(root_default)))
    vcpkg_root = Path(os.environ.get("VCPKG_ROOT", str(root / "vcpkg")))
    cache_dir = root

    check_port = "protobuf"

    triplet_default = _KIND_DEFAULT_VCPKG_TRIPLET.get(kind)
    if not triplet_default:
        raise RuntimeError(
            f"ensure_docker_deps: add kind {kind!r} to _KIND_DEFAULT_VCPKG_TRIPLET or set VCPKG_TRIPLET"
        )
    triplet = os.environ.get("VCPKG_TRIPLET", triplet_default)

    run_make_target = target if target in kind_run_make_targets(kind) else None
    run_make_extra_env: dict[str, str] = {}
    # Mark that the following Make invocation is running inside a docker/devcontainer build.
    # Makefile uses this to select docker/ vs host stamp locations.
    run_make_extra_env["IN_DOCKER"] = "1"

    # For docker kinds, keep stamp location in the standard docker cache tree.
    docker_stamp_dir = f"docker/{kind}/build"
    setup_stamp_file = f"{docker_stamp_dir}/.stamp_setup"

    return EnsurePlan(
        kind=kind,
        vcpkg_root=vcpkg_root,
        cache_dir=cache_dir,
        triplet=triplet,
        check_port=check_port,
        run_make_target=run_make_target,
        run_make_extra_env=run_make_extra_env,
        setup_stamp_file=setup_stamp_file,
    )


def run_make_if_requested(plan: EnsurePlan, make_args: list[str]) -> None:
    if not plan.run_make_target:
        return

    extra_env = dict(plan.run_make_extra_env or {})
    if plan.setup_stamp_file:
        repo_root = Path(__file__).resolve().parent.parent
        stamp_path = Path(plan.setup_stamp_file)
        if not stamp_path.is_absolute():
            stamp_path = repo_root / stamp_path
        touch_stamp_file(stamp_path)

    cmd = ["make", plan.run_make_target, *make_args]
    run_checked(cmd, env={**os.environ, **extra_env})


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("Usage: ensure_docker_deps.py <target> [make args...]", file=sys.stderr)
        return 1

    target = argv[1]
    make_args = argv[2:]

    script_dir = Path(__file__).resolve().parent

    last_step = "init"
    try:
        plan = build_ensure_plan(target)

        if not docker_deps_ready(plan):
            last_step = "deps setup (setup.sh)"
            # BUILD_DIR must match the parent of the setup stamp (e.g. docker/<kind>/build) so setup and build agree on REPO_BUILD_ROOT.
            build_dir_rel = plan.setup_stamp_file.rsplit("/", 1)[0]  # e.g. docker/<kind>/build
            setup_env: dict[str, str] = {
                "TARGET_PLATFORM": plan.kind,
                "VCPKG_DIR": str(plan.vcpkg_root),
                "VCPKG_TRIPLET": plan.triplet,
                "BUILD_DIR": build_dir_rel,
            }
            # QT_OUTPUT_DIR comes from devcontainer.json containerEnv / docker run -e.
            # deps-only: per TARGET_PLATFORM via setup.sh dispatch.
            setup_sh = script_dir / "setup.sh"
            # Only supported kinds are those in _KIND_DEFAULT_VCPKG_TRIPLET (see build_ensure_plan); all use `deps` here.
            setup_cmd = ["bash", str(setup_sh), "deps"]
            run_checked(
                setup_cmd,
                env={**os.environ, **setup_env},
                cwd=script_dir.parent,
            )
        last_step = "stamp and make"
        run_make_if_requested(plan, make_args)
        return 0
    except subprocess.CalledProcessError as e:
        print(
            f"ensure_docker_deps.py failed (exit {e.returncode}) at step={last_step} (target={target}).",
            file=sys.stderr,
        )
        return e.returncode
    except Exception as e:
        print(f"ensure_docker_deps.py failed at step={last_step} (target={target}): {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
