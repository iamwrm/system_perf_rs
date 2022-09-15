use clap::Parser;

use system_perf::get_rdtsc_ratio;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[clap(name = clap::crate_name!(), about = clap::crate_description!())]
#[clap(author = clap::crate_authors!(), version = clap::crate_version!())]
struct Args {
    /// Calculate to nth of the series sum
    #[clap(short, long, value_parser)]
    n: u32,
}

fn main() {
    let args = Args::parse();

    println!("vesrion: {}", clap::crate_version!());

    get_rdtsc_ratio(args.n);
}
