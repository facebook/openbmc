// SPDX-License-Identifier: GPL-2.0-only

use anyhow::{bail, Context, Result};
use clap::Parser;
use fdt;
use std::fs;
use std::io::Write;
use std::net::TcpListener;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use tempfile::NamedTempFile;

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about=None, set_term_width(0))]
struct Args {
    #[clap(short, long, env = "BUILDDIR", help = "Build directory")]
    build: Option<PathBuf>,

    #[clap(short, long, env = "MACHINE", help = "Machine name")]
    machine: Option<String>,

    #[clap(short, long, help = "QEMU directory")]
    qemu: Option<PathBuf>,

    #[clap(short, long, help = "Boot from flash, not the FIT image")]
    uboot: bool,

    #[clap(long, help = "Kernel command line (e.g. dts bootargs)")]
    bootargs: Option<String>,

    #[clap(long, help = "Enable userspace (slirp) network backend")]
    slirp: bool,

    #[clap(short, long, default_values_t = [22],
           help = "Ports to forward from the guest OS")]
    ports: Vec<u16>,

    #[clap(long, help = "Enable tap network backend")]
    tap: Option<String>,

}

fn parse_fit(fit: &[u8]) -> Result<Vec<(&str, &[u8])>> {
    let mut ret = Vec::new();
    let fit = match fdt::Fdt::new(fit) {
        Ok(fit) => fit,
        Err(e) => bail!("Unable to parse FIT image: {}", e),
    };
    let images = fit.find_node("/images").context("/images not found in FIT")?;
    for node in images.children() {
        let data = node.property("data")
            .context("FIT image component missing data property")?;
        println!("Found FIT component {}", node.name);
        ret.push((node.name, data.value));
    }
    Ok(ret)
}

fn write_temporary_file(data: &[u8]) -> Result<PathBuf> {
    let mut f = NamedTempFile::new()?;
    f.write_all(data)?;
    Ok(f.into_temp_path().keep()?)
}

fn add_fit_args(command: &mut Command, fit_path: &Path,
                bootargs: Option<&str>) -> Result<()> {
    let fit = fs::read(&fit_path).context("Error reading file")?;
    let fit_images = parse_fit(&fit).context("Error parsing FIT")?;
    for (name, data) in &fit_images {
        let path = write_temporary_file(data)?;
        let name = match *name {
            _ if name.starts_with("kernel") => "-kernel",
            _ if name.starts_with("ramdisk") => "-initrd",
            _ if name.starts_with("fdt") => "-dtb",
            _ => {
                println!("Unexpected FIT component: {}", name);
                continue;
            }
        };
        command.arg(name).arg(path);
    }
    if let Some(bootargs) = bootargs {
        command.arg("-append").arg(bootargs);
    }
    Ok(())
}

fn find_free_tcp_port() -> Result<u16> {
    let fd = TcpListener::bind(("127.0.0.1", 0))?;
    Ok(fd.local_addr()?.port())
}

fn print_qemu_command(command: &Command) {
    print!("{}", command.get_program().to_string_lossy());
    for arg in command.get_args() {
        let arg = arg.to_string_lossy();
        if arg.starts_with('-') {
            print!(" \\\n ");
        }
        print!(" {}", arg);
    }
    println!();
}

fn main() -> Result<()> {
    let args = Args::parse();
    let build_dir = args.build.context("Build directory undefined")?;
    let staging_bindir_native = option_env!("STAGING_BINDIR_NATIVE")
        .map(PathBuf::from);
    let qemu_dir = args.qemu.or(staging_bindir_native)
        .context("Unable to find QEMU")?;
    let machine = args.machine.context("Machine name undefined")?;

    let qemu_path = qemu_dir.join("qemu-system-arm");
    let mut command = Command::new(qemu_path);
    command.arg("-machine")
           .arg(format!("{}-bmc", machine))
           .arg("-nographic");

    let deploy_dir = build_dir.join(format!("tmp/deploy/images/{}/", machine));
    let mtd_path = deploy_dir.join(format!("flash-{}", machine));
    if mtd_path.exists() {
        println!("Using {} for primary and golden images.", mtd_path.display());
        for _ in 0..2 {
            command.arg("-drive")
                   .arg(format!("file={},format=raw,if=mtd,snapshot=on", mtd_path.display()));
        }
    }

    if !args.uboot {
        let search_paths = [
            deploy_dir.join(format!("fit-{machine}.itb", machine=machine)),
            deploy_dir.join(format!("fitImage-obmc-phosphor-initramfs-{machine}\
                                     -{machine}", machine=machine)),
        ];
        for fit_path in search_paths {
            if !fit_path.exists() {
                continue;
            }
            println!("Using {} as FIT image.", fit_path.display());
            add_fit_args(&mut command, &fit_path, args.bootargs.as_deref())
                .with_context(|| {
                    format!("Error loading FIT {}", fit_path.display())
                })?;
            break;
        }
    }

    if args.slirp {
        let mut s = String::from("user,id=nic,mfr-id=0x8119,\
                                  oob-eth-addr=fa:ce:b0:02:20:22");
        for guest_port in args.ports {
            let host_port = find_free_tcp_port()
                .context("Finding free TCP port")?;
            println!("Forwarding guest port {} to localhost:{}",
                     guest_port, host_port);
            s.push_str(&format!(",hostfwd=::{}-:{}", host_port, guest_port));
        }
        command.arg("-netdev")
               .arg(s)
               .arg("-net")
               .arg("nic,model=ftgmac100,netdev=nic");
    }

    if let Some(ifname) = args.tap {
        command.arg("-netdev")
               .arg(format!("tap,id=nic,ifname={},script=no,downscript=no", ifname))
               .arg("-net")
               .arg("nic,model=ftgmac100,netdev=nic");
    }

    print_qemu_command(&command);

    let mut child = command
        .stdin(Stdio::inherit())
        .stdout(Stdio::inherit())
        .stderr(Stdio::inherit())
        .spawn()
        .context("Error starting QEMU")?;
    child.wait().context("Error while running QEMU")?;
    Ok(())
}
