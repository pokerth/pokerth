#!/usr/bin/env python3
"""
Ensure vcpkg/Qt deps exist inside Docker for supported target kinds, then optionally run `make`.

Platform/kind agnosticism is a hard requirement: the logic must not special-case individual
platforms. The sole exception is _KIND_DEFAULT_VCPKG_TRIPLET, which provides a failsafe
default triplet per kind when VCPKG_TRIPLET is unset.
"""

from __future__ import annotations

import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


def run_checked(cmd: list[str], env: dict[str, str] | None = None, cwd: Path | None = None) -> None:
    subprocess.run(cmd, check=True, env=env, cwd=str(cwd) if cwd else None)


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


def chown_dir_recursively(path: Path, uid: str, gid: str) -> None:
    if not path.exists():
        return
    try:
        run_checked(["chown", "-R", f"{uid}:{gid}", str(path)])
    except subprocess.CalledProcessError:
        pass


def run_make_if_requested(plan: EnsurePlan, make_args: list[str]) -> None:
    if not plan.run_make_target:
        return

    cache_dir = plan.cache_dir
    extra_env = dict(plan.run_make_extra_env or {})
    if plan.setup_stamp_file:
        repo_root = Path(__file__).resolve().parent.parent
        stamp_path = Path(plan.setup_stamp_file)
        if not stamp_path.is_absolute():
            stamp_path = repo_root / stamp_path
        stamp_path = stamp_path.resolve()
        stamp_path.parent.mkdir(parents=True, exist_ok=True)
        stamp_path.touch()

    if int(subprocess.check_output(["id", "-u"], text=True).strip()) == 0:
        uid = os.environ.get("VSCODE_UID", "1000")
        gid = os.environ.get("VSCODE_GID", "1000")
        chown_dir_recursively(cache_dir, uid, gid)

        env_assignments: list[str] = [f"{k}={v}" for k, v in extra_env.items()]
        cmd = ["runuser", "-u", "vscode", "--", "env", *env_assignments, "make", plan.run_make_target, *make_args]
        run_checked(cmd)
    else:
        cmd = ["make", plan.run_make_target, *make_args]
        run_checked(cmd)


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

        vcpkg_not_ready = not vcpkg_ready(plan.vcpkg_root, plan.triplet, plan.check_port)
        if vcpkg_not_ready:
            last_step = "deps setup (setup.sh)"
            # BUILD_DIR must match stamp dir so setup_android.sh writes .android_env where build_android.sh expects it
            build_dir_rel = plan.setup_stamp_file.rsplit("/", 1)[0]  # e.g. docker/android/build
            setup_env: dict[str, str] = {
                "SKIP_QT_INSTALL": "yes",
                "SKIP_SYSTEM_PACKAGES": "yes",
                "TARGET_PLATFORM": plan.kind,
                "VCPKG_DIR": str(plan.vcpkg_root),
                "VCPKG_TRIPLET": plan.triplet,
                "BUILD_DIR": build_dir_rel,
            }
            # QT_OUTPUT_DIR comes from devcontainer.json containerEnv / docker run -e; Qt is baked in image.
            setup_sh = script_dir / "setup.sh"
            run_checked(
                ["bash", str(setup_sh)],
                env={**os.environ, **setup_env},
                cwd=script_dir.parent,
            )
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
