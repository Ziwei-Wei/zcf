void
Session::close(
    std::string reason
    )
{
    log(
        std::move(reason)
        );
    pending_.erase(
        id_
        );
    notify(
        State::Closed
        );
    auto timeout = std::chrono::milliseconds(
        250
        );
    report(
        id_,
        timeout
        );
    write(
        "session "
        "closed"
        );
}
