use clap::Parser;

use system_perf::get_rdtsc_ratio;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    /// length to compute
    #[clap(short, long, value_parser, default_value_t = 1)]
    length: u32,
}

fn main() {
    let args = Args::parse();

    get_rdtsc_ratio(args.length);
}
