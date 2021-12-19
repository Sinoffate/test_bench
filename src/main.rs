use nix::ioctl_readwrite;
use std::env;
use std::io::prelude::*;
use std::os::unix::io::AsRawFd;

const IOCTL_MEME_BASE: u8 = 0x69;
const IOCTL_MEME_INCREMENT: u8 = 0x0;

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct meme_increment_t {
    pub target: u64,
}

ioctl_readwrite!(
    meme_increment,
    IOCTL_MEME_BASE,
    IOCTL_MEME_INCREMENT,
    meme_increment_t
);

fn main() {
    println!("Usage ./test_bench /location/to/device <input number>");

    let args: Vec<String>= env::args().collect();
    let path = args[1].clone();
    let val = args[2].parse().unwrap();

    let mut driver_fd = std::fs::OpenOptions::new()
        .read(true)
        .write(true)
        .open(path)
        .expect("couldn't open driver path");

    let mut output = String::new();
    driver_fd.read_to_string(&mut output).unwrap();

    if output != "Hello world!" {
        println!("lol get fucked - {}", output);
    }

    let mut input = meme_increment_t {
        target: val,
    };

    unsafe {
        meme_increment(driver_fd.as_raw_fd(), &mut input).expect("call to ioctl failed...");
    }

    println!("got input {}, expected result {}, actual result {}", val, val + 1, input.target);
}

