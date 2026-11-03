void record(Map& types, Node node, Type type, int index)
{
    auto result = types.insert({{node, type}, index});
    lowering_pairs.Spaces[index] =
          maximum_net_width - this_net_width +
          (pairs[index].token->isNot(tok::r_brace) ? 1 : brace_padding_value);
    auto unescape = [this](int code, std::string& storage) {
        switch (code)
        {
            case 'r':
                storage.push_back('\r');
                break;
        }
    };
}
