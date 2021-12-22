use nix::{ioctl_read, ioctl_readwrite, ioctl_write_ptr};
use std::io::prelude::*;
use std::os::unix::io::AsRawFd;
use std::path::PathBuf;
use structopt::StructOpt;

#[derive(Debug, StructOpt)]
struct Opt {
    /// Path to the device driver [/dev/meme]
    #[structopt(parse(from_os_str))]
    path: PathBuf,

    /// Test device read functionality returns "Hello world!"
    #[structopt(short)]
    read: bool,

    /// Test whether driver can increment an input value using an ioctl
    #[structopt(short = "i", long = "inc", name = "int")]
    inc: Option<u64>,
}

const IOCTL_MEME_BASE: u8 = 0x69;
const IOCTL_MEME_INCREMENT: u8 = 0x0;
const IOCTL_MEME_SET_VAL: u8 = 0x01;
const IOCTL_MEME_GET_VAL: u8 = 0x02;

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct meme_increment_t {
    pub target: u64,
}

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct meme_set_val_t {
    pub key: u64,
    pub payload: *const u8,
}

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct meme_get_val_t {
    pub key: u64,
    pub payload: *const u8,
}

ioctl_readwrite!(
    meme_increment,
    IOCTL_MEME_BASE,
    IOCTL_MEME_INCREMENT,
    meme_increment_t
);

ioctl_write_ptr!(
    meme_set_val,
    IOCTL_MEME_BASE,
    IOCTL_MEME_SET_VAL,
    meme_set_val_t
);

ioctl_read!(
    meme_get_val,
    IOCTL_MEME_BASE,
    IOCTL_MEME_GET_VAL,
    meme_get_val_t
);

fn main() {
    let opt = Opt::from_args();

    let mut driver_fd = std::fs::OpenOptions::new()
        .read(true)
        .write(true)
        .open(opt.path)
        .expect("couldn't open driver path");

    if opt.read {
        let mut output = String::new();
        driver_fd.read_to_string(&mut output).unwrap();

        if output != "Hello world!" {
            println!("lol get fucked - {}", output);
        }
    }

    if let Some(val) = opt.inc {
        let mut input = meme_increment_t { target: val };

        unsafe {
            meme_increment(driver_fd.as_raw_fd(), &mut input).unwrap();
        }

        println!(
            "got input {}, expected result {}, actual result {}",
            val,
            val + 1,
            input.target
        );
    }
}
