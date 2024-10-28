template <typename D>
bool JinjaComparable<D>::IsEqual(const jinja2::IComparable &other) const
{
    if (auto *omsg = dynamic_cast<const D *>(&other))
        return *static_cast<const D *>(this) == *omsg;
    return false;
}

template <typename D>
jinja2::Value JinjaHelper<D>::GetValueByName(const std::string &name) const
{
    if (auto it = D::s_fields.find(name); it != D::s_fields.end()) {
        auto [_, func] = *it;
        return func(static_cast<const D *>(this)->value());
    }
    return jinja2::EmptyValue();
}
