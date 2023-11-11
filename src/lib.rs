// This is the main entry point for the library.

mod my_module;

pub fn add_numbers(a: i32, b: i32) -> i32 {
    my_module::add_internal(a, b)
}