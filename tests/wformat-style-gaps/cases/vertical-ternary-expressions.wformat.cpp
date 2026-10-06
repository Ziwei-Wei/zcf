extern bool condition_with_an_extremely_long_descriptive_name;
extern int true_value_with_an_extremely_long_descriptive_name;
extern int false_value_with_an_extremely_long_descriptive_name;
extern int another_argument_with_a_very_long_name_that_forces_the_outer_call_to_wrap;
extern int state;

int
combine(
    int first,
    int second
    );

int
update();

int
choose_value(
    bool condition,
    int true_value,
    int false_value
    )
{
    const int compact = condition ? true_value : false_value;
    const int compact_in_wrapped_call = combine(
        condition ? true_value : false_value,
        another_argument_with_a_very_long_name_that_forces_the_outer_call_to_wrap
        );
    const int nested_assignment_condition =
        ((state = update()) != 0 && condition_with_an_extremely_long_descriptive_name)
        ?
        true_value_with_an_extremely_long_descriptive_name
        :
        false_value_with_an_extremely_long_descriptive_name;
    const int expanded_result_with_a_long_descriptive_name =
        condition_with_an_extremely_long_descriptive_name
        ?
        true_value_with_an_extremely_long_descriptive_name
        :
        false_value_with_an_extremely_long_descriptive_name;

    return compact + compact_in_wrapped_call + nested_assignment_condition + expanded_result_with_a_long_descriptive_name;
}
