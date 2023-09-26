#include <pybind11/embed.h>  // everything needed for embedding

#include <fstream>
#include <string>
namespace py = pybind11;
using namespace py::literals;

int main() {
  py::scoped_interpreter guard{};  // start the interpreter and keep it alive

  printf(
      "++++++++++PLEASE MAKE SURE YOU ARE IN THE PYBIND_SAMPLE DIRECTORY WHEN "
      "RUNNING THE BINARY+++++++++\n");
  printf(
      "This is so the program correctly finds the Python files in the "
      "pybind_sample directory\n\n");

  printf("We can read a Python file into a string and execute its entirety\n");
  std::ifstream ifs("print_sample.py", std::ifstream::in);
  std::string str = "";
  char c = ifs.get();
  while (ifs.good()) {
    str += c;
    c = ifs.get();
  }
  py::exec(str);
  ifs.close();

  printf("\nWe can call a Python module from a file\n");
  py::object sdk_validator = py::module_::import("validator_example");
  py::object validate = sdk_validator.attr("validate");
  py::object res = validate("some_string");

  if (res == py::bool_(true))
    printf("validate returned true\n");
  else
    printf("validate returned false\n");

  printf("\nWe can also mix direct execution and the C++ library\n");
  auto txn =
      py::dict("id"_a = "918544363", "asset"_a = py::dict("id"_a = "1111"));
  auto locals = py::dict("name"_a = "World", "number"_a = 42, "txn"_a = txn);
  py::exec(R"(
    def validate(transaction):
      print(f'Validating {transaction}')

      if (not transaction) or type(transaction) is not dict:
        return False
      if "id" not in transaction:
        return False
      if "asset" not in transaction:
        return False
      if "id" not in transaction["asset"]:
        return False
      if transaction["id"] == None or transaction["asset"] == None:
        return False
      return True
    is_valid = validate(txn)
    print(f"validate returned {is_valid}")
  )",
           py::globals(), locals);

  auto is_valid = locals["is_valid"].cast<bool>();

  if (is_valid) printf("Valid\n");
}
