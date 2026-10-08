void
run()
{
    first = 1;
    much_longer_assignment_name = 2;
    result = outer_function(short_value, nested_function(first_value, second_value), final_value);
    result = outer_function(
        short_value,
        nested_function(first_value, second_value),
        final_value
        );
    result = outer_function(short_value, nested_function(first_value_with_a_very_long_name, second_value_with_a_very_long_name), final_value);
    result = outer_function(short_value, nested_function(first_value_with_a_very_long_name, deeper_function(alpha_value_with_a_very_long_name, beta_value_with_a_very_long_name)), final_value);
    ZCF_CALL(short_value, nested_function(first_value_with_a_very_long_name, second_value_with_a_very_long_name), macro_final_value_with_a_very_long_name);
}
