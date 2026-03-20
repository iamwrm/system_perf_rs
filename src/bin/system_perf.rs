use clap::{Parser, Subcommand};

use system_perf::branch_prediction::run_branch_prediction_benchmark;
use system_perf::launch_threads;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[clap(name = clap::crate_name!(), about = clap::crate_description!())]
#[clap(author = clap::crate_authors!(), version = clap::crate_version!())]
struct Args {
    /// Calculate to nth of the series sum
    #[clap(short, long, value_parser)]
    n: Option<i32>,
    /// bench iterations
    #[clap(short, long, value_parser, default_value_t = 10_000_000u64)]
    iter_time: u64,
    #[clap(short, long)]
    cores: Option<String>,

    #[clap(subcommand)]
    command: Option<Commands>,
}

#[derive(Subcommand, Debug)]
enum Commands {
    /// Run branch prediction benchmark (Lemire)
    BranchPrediction {
        /// Maximum number of values to test (default: 1000000)
        #[clap(short, long, default_value_t = 1_000_000)]
        max_size: usize,
    },
}

fn get_core_list(cores: &str) -> Vec<usize> {
    let mut ans = vec![];
    for i in cores.split(',') {
        if i.contains('-') {
            let mut iter = i.split('-');
            let start = iter.next().unwrap().parse::<usize>().unwrap();
            let end = iter.next().unwrap().parse::<usize>().unwrap();
            for j in start..=end {
                ans.push(j);
            }
        } else {
            ans.push(i.parse::<usize>().unwrap());
        }
    }
    ans
}
#[test]
fn test_get_core_list() {
    assert_eq!(get_core_list("1"), vec![1]);
    assert_eq!(get_core_list("1,2,3"), vec![1, 2, 3]);
    assert_eq!(get_core_list("1-3"), vec![1, 2, 3]);
    assert_eq!(get_core_list("1-3,5"), vec![1, 2, 3, 5]);
    assert_eq!(get_core_list("1-3,7-10,11"), vec![1, 2, 3, 7, 8, 9, 10, 11]);
    assert_eq!(get_core_list("1-3,2-4"), vec![1, 2, 3, 2, 3, 4]);
}

fn main() {
    let args = Args::parse();

    match args.command {
        Some(Commands::BranchPrediction { max_size }) => {
            run_branch_prediction_benchmark(max_size);
        }
        None => {
            let n = args.n.expect("Error: -n is required when running the default Taylor series benchmark");
            dbg!(&args.iter_time);
            dbg!(&n);

            println!("version: {}", clap::crate_version!());

            let core_list = args.cores.map(|cores| get_core_list(&cores));

            launch_threads(n, args.iter_time, core_list);
        }
    }
}
