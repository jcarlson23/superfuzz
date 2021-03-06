#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#include "option.h"
#include "type.h"

typedef std::mt19937 generator_type;
static generator_type generator;
static Option<unsigned long> seed("seed", generator_type::default_seed);
static Option<int> num_classes("num-classes", 30);
static Option<int> min_num_fields("min-num-fields", 0);
static Option<int> max_num_fields("max-num-fields", 30);
static Option<int> avg_num_array_elements("avg-num-array-elements", 3);
static Option<int> chance_of_ctor("chance-of-ctor", 90);
static Option<int> chance_of_base("chance-of-base", 5);
static Option<int> chance_of_vbase("chance-of-vbase", 30);
static Option<int> chance_of_array("chance-of-array", 15);
static Option<int> chance_of_anon_field("chance-of-anon-field", 40);
static Option<int> chance_of_bitfield("chance-of-bitfield", 10);
static Option<int> chance_of_own_method("chance-of-own-method", 20);
static Option<int> chance_of_override_method("chance-of-override-method", 20);
static Option<int> chance_of_virt_override("chance-of-virt-override", 60);
static Option<int> chance_of_pure_virt("chance-of-pure-virt", 20);
static Option<int> chance_of_class_aligned("chance-of-class-aligned", 10);
static Option<int> chance_of_class_packed("chance-of-class-packed", 10);
static Option<int> chance_of_class_vtordisp("chance-of-vtordisp-packed", 10);
static Option<int> chance_of_field_aligned("chance-of-field-aligned", 10);
static Option<bool> check_vptrs("check-vptrs", false);
static Option<bool> gnu_dialect("gnu-dialect", false);
static Option<bool> show_help("help", false);

int main(int argc, const char *argv[]) {
  parse_options(argc, argv);

  if (show_help) {
    usage(argv[0]);
    return EXIT_SUCCESS;
  }

  generator.seed((unsigned long)seed);

  std::uniform_int_distribution<int> field_count_dist(min_num_fields,
                                                      max_num_fields);
  std::poisson_distribution<int> field_array_elt_dist(avg_num_array_elements);
  std::uniform_int_distribution<int> percent(1, 100);
  std::uniform_int_distribution<int> field_alignment_pow2(0, 13);
  std::uniform_int_distribution<int> class_alignment_pow2(0, 13);
  std::uniform_int_distribution<int> class_packed_pow2(0, 4);
  std::uniform_int_distribution<int> class_vtordisp(0, 2);

  if (!check_vptrs) {
    std::cout << "#if defined(__clang__) || defined(__GNUC__)\n";
    std::cout << "typedef __SIZE_TYPE__ size_t;\n";
    std::cout << "#endif\n";
    std::cout << "extern \"C\" int printf(const char *, ...);\n";
    std::cout << "extern \"C\" void *memset(void *, int, size_t);\n";
    std::cout << "static char buffer[419430400];\n";
    std::cout << "inline void *operator new(size_t, void *pv) { return pv; }\n";
  }
  auto shuffled_classes = new int[num_classes];
  for (int class_i = 0; class_i < num_classes; ++class_i) {
    auto new_type = new Class(class_i);
    if (int num_pbases = types.size()) {
      // fill shuffled_classes with the range [0, num_pbases]
      for (int pbase_i = 0; pbase_i < num_pbases; ++pbase_i) {
        shuffled_classes[pbase_i] = pbase_i;
      }
      // randomize the order of which potential bases to inherit from
      std::shuffle(shuffled_classes, shuffled_classes + num_pbases, generator);
      for (int pbase_i = 0; pbase_i < num_pbases; ++pbase_i) {
        if (percent(generator) > chance_of_base) {
          continue;
        }

        int pbase = shuffled_classes[pbase_i];
        if (!gnu_dialect && !new_type->is_viable_base(pbase)) {
          continue;
        }

        bool is_virtual = percent(generator) <= chance_of_vbase;
        new_type->add_base(pbase, is_virtual);
      }
    }

    types.push_back(new_type);
    int num_fields = field_count_dist(generator);
    for (int field_i = 0; field_i < num_fields; ++field_i) {
      std::uniform_int_distribution<int> field_type_dist(
          TypeKind_Bool, types.empty() ? TypeKind_Double : TypeKind_Class);
      int field_type = field_type_dist(generator);
      int type_class = -1;
      if (field_type >= TypeKind_PClass) {
        if (field_type == TypeKind_Class && types.size() == 1)
          field_type = TypeKind_PClass;
        std::uniform_int_distribution<int> type_dist(
            0,
            field_type == TypeKind_Class ? types.size() - 2 : types.size() - 1);
        type_class = type_dist(generator);
      }
      auto &field = new_type->add_field((TypeKind)field_type);
      field.set_type_class(type_class);

      if (field_type <= TypeKind_LongLong &&
          percent(generator) <= chance_of_bitfield) {
        int bitfield_width = field_array_elt_dist(generator);
        field.set_bitfield_width(bitfield_width);
        if (percent(generator) <= chance_of_anon_field) {
          field.set_anonymous();
        }
      } else if (percent(generator) <= chance_of_array) {
        do {
         field.add_array_dimension(field_array_elt_dist(generator) + 1);
        } while (percent(generator) <= chance_of_array);
      }
      if (percent(generator) <= chance_of_field_aligned) {
        do {
          int field_alignment = 1 << field_alignment_pow2(generator);
          field.set_alignment(field_alignment, gnu_dialect);
        } while (percent(generator) <= chance_of_field_aligned);
      }
    }
    std::uniform_int_distribution<int> ret_type_dist(
        TypeKind_Bool, types.empty() ? TypeKind_Double : TypeKind_Class);
    if (percent(generator) <= chance_of_own_method) {
      int ret_type = ret_type_dist(generator);
      int ret_type_class = -1;
      if (ret_type >= TypeKind_PClass) {
        if (ret_type == TypeKind_Class && types.size() == 1)
          ret_type = TypeKind_PClass;
        std::uniform_int_distribution<int> ret_type_class_dist(
            0,
            ret_type == TypeKind_Class ? types.size() - 2 : types.size() - 1);
        ret_type_class = ret_type_class_dist(generator);
      }
      Class::Method method;
      method.name = new_type->get_class_name() + "Method";
      method.ret_type = (TypeKind)ret_type;
      method.ret_type_class = ret_type_class;
      method.is_virtual = true;
      method.arg_type = TypeKind_Bool;
      method.arg_type_class = -1;
      new_type->add_method(method);
    }
    if (percent(generator) <= chance_of_override_method) {
      int ret_type = ret_type_dist(generator);
      int ret_type_class = -1;
      if (ret_type >= TypeKind_PClass) {
        if (ret_type == TypeKind_Class && types.size() == 1)
          ret_type = TypeKind_PClass;
        std::uniform_int_distribution<int> ret_type_class_dist(
            0,
            ret_type == TypeKind_Class ? types.size() - 2 : types.size() - 1);
        ret_type_class = ret_type_class_dist(generator);
      }
      Class::Method method;
      method.name = "OverrideMethod";
      method.ret_type = (TypeKind)ret_type;
      method.ret_type_class = ret_type_class;
      method.is_virtual = percent(generator) <= chance_of_virt_override;
      method.is_pure = method.is_virtual && percent(generator) <= chance_of_pure_virt;
      int arg_type = ret_type_dist(generator);
      int arg_type_class = -1;
      if (arg_type >= TypeKind_PClass) {
        if (arg_type == TypeKind_Class && types.size() == 1)
          arg_type = TypeKind_PClass;
        std::uniform_int_distribution<int> arg_type_class_dist(
            0,
            arg_type == TypeKind_Class ? types.size() - 2 : types.size() - 1);
        arg_type_class = arg_type_class_dist(generator);
      }
      method.arg_type = (TypeKind)arg_type;
      method.arg_type_class = arg_type_class;
      new_type->add_method(method);
    }
    if (percent(generator) <= chance_of_class_packed) {
      int packed = 1 << class_packed_pow2(generator);
      new_type->set_packed(packed);
    }
    new_type->set_dllexport(check_vptrs);
    new_type->set_ctor(percent(generator) <= chance_of_ctor);
    if (percent(generator) <= chance_of_class_vtordisp) {
      int vtordisp = class_vtordisp(generator);
      new_type->set_vtordisp(vtordisp);
    }
    if (percent(generator) <= chance_of_class_aligned) {
      int align = 1 << class_alignment_pow2(generator);
      new_type->set_alignment(align, gnu_dialect);
    }
  }

  for (int class_i = 0; class_i < num_classes; ++class_i)
    std::cout << *types[class_i];

  if (!check_vptrs) {
    std::cout << "static void test_layout(const char *class_name, size_t size_of_class, size_t align_of_class) {\n";
    if (gnu_dialect) {
      std::cout << "\tprintf(\"     sizeof(%s): %zu\\n\", class_name, size_of_class);\n";
      std::cout << "\tprintf(\"__alignof__(%s): %zu\\n\", class_name, align_of_class);\n";
    } else {
      std::cout << "\tprintf(\"   sizeof(%s): %Iu\\n\", class_name, size_of_class);\n";
      std::cout << "\tprintf(\"__alignof(%s): %Iu\\n\", class_name, align_of_class);\n";
    }
    std::cout << "}\n";

    std::cout << "template <typename Class>\n";
    std::cout << "static void init_mem() {\n";
    std::cout << "\tmemset(buffer, 0xcc, sizeof(buffer));\n";
    std::cout << "\tnew (buffer) Class;\n";
    std::cout << "}\n";

    std::cout << "#define test(Class) init_mem<Class>(), test_layout(#Class, sizeof(Class), __alignof(Class))\n";

    std::cout << "int main() {\n";


    // fill shuffled_classes with the range [0, num_pbases]
    for (int class_i = 0; class_i < num_classes; ++class_i) {
      shuffled_classes[class_i] = class_i;
    }
    // randomize the order of which potential bases to inherit from
    std::shuffle(shuffled_classes, shuffled_classes + num_classes, generator);
    for (int class_i = 0; class_i < num_classes; ++class_i) {
      std::cout << "\ttest(" << types[shuffled_classes[class_i]]->get_class_name() << ");\n";
    }

    std::cout << "}\n";
  }

  return EXIT_SUCCESS;
}
