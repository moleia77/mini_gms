#include "mini_gms_doc.hpp"
#include "mini_gms_parser.hpp"

namespace mol {

    ValuePtr Value::get(const std::string& key) const {
        if (!isStu()) return nullptr;
        for (auto& kv : asStu())
            if (kv.first == key) return kv.second;
        return nullptr;
    }

    std::vector<PathToken> parsePath(const std::string& path) {
        std::vector<PathToken> out;
        size_t i = 0;
        const size_t n = path.size();

        while (i < n) {
            if (path[i] == '.') {
                ++i;
                continue;
            }

            if (path[i] == '[') {
                ++i;
                size_t start = i;
                while (i < n && std::isdigit(static_cast<unsigned char>(path[i])))
                    ++i;
                if (i >= n || path[i] != ']' || i == start) {
                    throw Error("invalid array index in path: " + path);
                }
                PathToken t;
                t.isIndex = true;
                t.index = static_cast<size_t>(std::stoul(path.substr(start, i - start)));
                out.push_back(t);
                ++i;
                continue;
            }

            size_t start = i;
            while (i < n && path[i] != '.' && path[i] != '[')
                ++i;
            if (i > start) {
                PathToken t;
                t.isIndex = false;
                t.name = path.substr(start, i - start);
                out.push_back(t);
            }
        }
        return out;
    }

    ValuePtr walk(const ValuePtr& root, const std::vector<PathToken>& tokens) {
        ValuePtr cur = root;
        for (const auto& t : tokens) {
            if (!cur)
                return nullptr;

            if (t.isIndex) {
                if (!cur->isVec())
                    return nullptr;
                const auto& vec = cur->asVec();
                if (t.index >= vec.size())
                    return nullptr;
                cur = vec[t.index];
            }
            else {
                if (!cur->isStu())
                    return nullptr;
                cur = cur->get(t.name);
            }
        }
        return cur;
    }

    MgmsDoc::MgmsDoc(std::vector<std::pair<std::string, ValuePtr>> ast)
        : ast_(std::move(ast))
    {
    }

    MgmsDoc MgmsDoc::parse(const std::string& src) {
        return MgmsDoc(mgms::parse(src));
    }

    ValuePtr MgmsDoc::walkFromTop(const std::vector<PathToken>& tokens) const {
        if (tokens.empty())
            return nullptr;

        const auto& first = tokens[0];
        if (first.isIndex)
            return nullptr;

        for (const auto& kv : ast_) {
            if (kv.first == first.name) {
                std::vector<PathToken> rest(tokens.begin() + 1, tokens.end());
                return rest.empty() ? kv.second : walk(kv.second, rest);
            }
        }
        return nullptr;
    }

    bool MgmsDoc::has(const std::string& path) const {
        return node(path) != nullptr;
    }

    ValuePtr MgmsDoc::node(const std::string& path) const {
        return walkFromTop(parsePath(path));
    }

    Vec MgmsDoc::getVec(const std::string& path) const {
        auto v = node(path);
        if (!v || !v->isVec())
            return Vec{};
        return v->asVec();
    }

    size_t MgmsDoc::size(const std::string& path) const {
        auto v = node(path);
        if (!v)
            return 0;
        if (v->isVec())
            return v->asVec().size();
        if (v->isStu())
            return v->asStu().size();
        return 0;
    }

    ValuePtr MgmsDoc::at(const std::string& path, size_t i) const {
        auto v = node(path);
        if (!v || !v->isVec())
            return nullptr;
        const auto& vec = v->asVec();
        if (i >= vec.size())
            return nullptr;
        return vec[i];
    }

}