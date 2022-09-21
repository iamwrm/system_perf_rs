use clap::Parser;

use system_perf::launch_threads;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[clap(name = clap::crate_name!(), about = clap::crate_description!())]
#[clap(author = clap::crate_authors!(), version = clap::crate_version!())]
struct Args {
    /// Calculate to nth of the series sum
    #[clap(short, long, value_parser)]
    n: i32,
    /// bench iterations
    #[clap(short, long, value_parser, default_value_t = 10_000_000u64)]
    iter_time: u64,
    #[clap(short, long)]
    cores: Option<String>,
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
}

fn main() {
    let args = Args::parse();

    dbg!(&args);

    println!("vesrion: {}", clap::crate_version!());

    let core_list = args.cores.map(|cores| get_core_list(&cores));

    launch_threads(args.n, args.iter_time, core_list);
}
