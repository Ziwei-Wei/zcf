void resolve(Callback send_result)
{
    symbol.lookup_async([this, jd = std::move(info.jd), send_result = std::move(send_result)](
                            Expected<Symbol> result) mutable {
        if (result)
        {
            send_result(std::move(result));
        }
        else
        {
            send_result(std::move(result));
        }
    });
}
