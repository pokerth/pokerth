#!/usr/bin/env python3
"""
Build a Docker image from a devcontainer.json and run `./scripts/ensure_docker_deps.py`.

The Docker build/run plan (dockerfile/build context, mounts, workdir) is derived
from the matching devcontainer.json based on the target.
"""

from __future__ import annotations

import json
import platform
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


def strip_devcontainer_json_comments(text: str) -> str:
    """
    Remove devcontainer // comments so we can JSON-parse.

    - Drop full-line comments: ^\\s*//.*
    - Strip trailing inline comments when // is preceded by whitespace: \\s+//.*
    """

    out_lines: list[str] = []
    for line in text.splitlines(True):
        if re.match(r"^\s*//.*$", line):
            continue
        line = re.sub(r"\s+//.*$", "", line)
        out_lines.append(line)
    return "".join(out_lines)


def parse_kv_list(spec: str) -> dict[str, str]:
    """
    Parse mount spec chunks like:
      source=/a,target=/b,type=bind,consistency=cached
    """

    out: dict[str, str] = {}
    for part in spec.split(","):
        part = part.strip()
        if not part or "=" not in part:
            continue
        key, val = part.split("=", 1)
        out[key.strip()] = val.strip()
    return out


def placeholder_substitute(val: str, local_workspace_folder: str) -> str:
    return val.replace("${localWorkspaceFolder}", local_workspace_folder)


def load_devcontainer_json(path: Path) -> dict[str, Any]:
    raw = path.read_text(encoding="utf-8")
    stripped = strip_devcontainer_json_comments(raw)
    return json.loads(stripped)


@dataclass(frozen=True)
class DevcontainerPlan:
    dockerfile_abs: str
    build_context_abs: str
    build_target: str | None
    build_options: list[str]  # devcontainer.json build.options (containers.dev)
    workdir: str
    run_mounts: list[str]  # each is host:dest[:cached]
    container_env: list[str]  # each is KEY=VAL


def ensure_host_path_for_mount(source: str) -> None:
    """
    Ensure a mount source exists on the host.

    If this is a socket (docker.sock), don't create the file, just ensure parent exists.
    """

    p = Path(source)
    if p.exists():
        return

    if p.name == "docker.sock" or p.suffix == ".sock":
        p.parent.mkdir(parents=True, exist_ok=True)
        return

    p.mkdir(parents=True, exist_ok=True)


def devcontainer_plan_from_json(
    devcontainer_json_path: Path,
    local_workspace_folder: str,
) -> DevcontainerPlan:
    devcontainer_json = load_devcontainer_json(devcontainer_json_path)
    devcontainer_dir = devcontainer_json_path.parent.resolve()

    build = devcontainer_json.get("build") or {}
    dockerfile_rel = build.get("dockerfile")
    context_rel = build.get("context")
    build_target = build.get("target")
    raw_opts = build.get("options") or []
    if not isinstance(raw_opts, list):
        raise RuntimeError(f"{devcontainer_json_path}: build.options must be an array of strings when set")
    build_options = [str(x) for x in raw_opts]

    if not dockerfile_rel or not context_rel:
        raise RuntimeError(
            f"{devcontainer_json_path}: missing build.dockerfile/context; cannot determine docker build args."
        )

    dockerfile_abs = (devcontainer_dir / str(dockerfile_rel)).resolve().as_posix()
    build_context_abs = (devcontainer_dir / str(context_rel)).resolve().as_posix()
    build_target = str(build_target) if build_target not in (None, "") else None

    workspace_folder = devcontainer_json.get("workspaceFolder") or None
    workspace_mount_str = devcontainer_json.get("workspaceMount")
    if not workspace_mount_str:
        raise RuntimeError(f"{devcontainer_json_path}: missing workspaceMount")

    workspace_mount_str = placeholder_substitute(str(workspace_mount_str), local_workspace_folder)
    workspace_parts = parse_kv_list(workspace_mount_str)

    ws_source = workspace_parts.get("source")
    ws_target = workspace_parts.get("target")
    if not ws_source or not ws_target:
        raise RuntimeError(f"{devcontainer_json_path}: workspaceMount missing source/target")

    ws_source_abs = Path(ws_source).resolve().as_posix()
    workdir = str(workspace_folder) if workspace_folder else str(ws_target)

    ws_suffix = ":cached" if workspace_parts.get("consistency") == "cached" else ""
    run_mounts: list[str] = [f"{ws_source_abs}:{ws_target}{ws_suffix}"]

    for mount_spec in devcontainer_json.get("mounts") or []:
        spec_str = placeholder_substitute(str(mount_spec), local_workspace_folder)
        parts = parse_kv_list(spec_str)
        src = parts.get("source")
        tgt = parts.get("target")
        if not src or not tgt:
            continue

        src_abs = Path(src).resolve().as_posix()
        suffix = ":cached" if parts.get("consistency") == "cached" else ""
        ensure_host_path_for_mount(src_abs)
        run_mounts.append(f"{src_abs}:{tgt}{suffix}")

    container_env_obj = devcontainer_json.get("containerEnv") or {}
    if not isinstance(container_env_obj, dict):
        raise RuntimeError(f"{devcontainer_json_path}: containerEnv must be an object")

    container_env = [f"{k}={v}" for k, v in container_env_obj.items()]

    return DevcontainerPlan(
        dockerfile_abs=dockerfile_abs,
        build_context_abs=build_context_abs,
        build_target=build_target,
        build_options=build_options,
        workdir=workdir,
        run_mounts=run_mounts,
        container_env=container_env,
    )


def echo_cmd(cmd: list[str]) -> None:
    print(" ".join(str(x) for x in cmd))


def run_checked(cmd: list[str]) -> None:
    echo_cmd(cmd)
    subprocess.run(cmd, check=True)


def dedup_mounts_by_dest(mounts: list[str]) -> list[str]:
    # mount format we generate: HOST:DEST[:cached]
    # Keep the last mount per destination path.
    by_dest: dict[str, str] = {}
    order: list[str] = []
    for m in mounts:
        core = m[:-7] if m.endswith(":cached") else m
        dest = core.split(":", 1)[1]
        if dest not in by_dest:
            order.append(dest)
        by_dest[dest] = m
    return [by_dest[d] for d in order]


def main(argv: list[str]) -> int:
    import argparse

    parser = argparse.ArgumentParser(
        prog="run_devcontainer.py",
        description="Build docker image + run ensure_docker_deps.py + make target inside container.",
    )
    parser.add_argument("image")
    parser.add_argument("dockerfile")
    parser.add_argument("make_target")
    parser.add_argument("--target", dest="build_target_opt", default=None)
    parser.add_argument(
        "--mount",
        dest="cli_mount_specs",
        action="append",
        default=[],
        help="Extra mount spec in HOST:GUEST form (optionally include :cached). Can be repeated.",
    )
    parser.add_argument(
        "-e",
        dest="env_cli",
        action="append",
        default=[],
        metavar="KEY=VAL",
        help="Extra container environment variable. Can be repeated.",
    )

    args = parser.parse_args(argv[1:])

    image: str = args.image
    make_target: str = args.make_target
    env_cli: list[str] = list(args.env_cli)
    build_target_opt: str | None = args.build_target_opt
    cli_mount_specs: list[str] = list(args.cli_mount_specs)

    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent.resolve()

    # Derive "kind" from the first token of the target (before the first '-').
    # Example: foo-bar -> kind foo
    kind = make_target.split("-", 1)[0]

    devcontainer_path = repo_root / "docker" / kind / ".devcontainer" / "devcontainer.json"
    if not devcontainer_path.exists():
        raise RuntimeError(f"Unsupported devcontainer target: {make_target}")
    local_workspace_folder = (repo_root / "docker" / kind).as_posix()

    devcontainer_json_for_runargs = load_devcontainer_json(devcontainer_path)
    run_args = devcontainer_json_for_runargs.get("runArgs")
    # runArgs: docker run (always). docker build: use build.options when set (containers.dev); else runArgs (legacy).
    run_opts: list[str] = run_args if isinstance(run_args, list) else []

    plan = devcontainer_plan_from_json(devcontainer_path, local_workspace_folder=local_workspace_folder)

    docker_build_extras = plan.build_options if plan.build_options else run_opts

    docker_cmd = [
        "docker",
        "build",
        "-f",
        plan.dockerfile_abs,
        "-t",
        image,
        *docker_build_extras,
        plan.build_context_abs,
    ]
    effective_build_target = build_target_opt or plan.build_target
    if effective_build_target:
        docker_cmd += ["--target", effective_build_target]

    run_mounts = dedup_mounts_by_dest(plan.run_mounts + cli_mount_specs)
    run_env = env_cli + plan.container_env

    vol_args: list[str] = [item for m in run_mounts for item in ("-v", m)]
    env_args: list[str] = [item for kv in run_env for item in ("-e", kv)]

    run_cmd = [
        "docker",
        "run",
        "--rm",
        *run_opts,
        "--user",
        "root",
        *vol_args,
        *env_args,
        "-w",
        plan.workdir,
        image,
        "./scripts/ensure_docker_deps.py",
        make_target,
    ]

    run_checked(docker_cmd)
    run_checked(run_cmd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

