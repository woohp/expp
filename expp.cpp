#include "expp.hpp"
#include <atomic>
#include <memory_resource>
#include <thread>

using namespace std;
using namespace expp;


vector<int> vector_times_int(vector<int> v, int i)
{
    for (auto& x : v)
        x *= i;
    return v;
}


vector<bool> bool_vector_negate(vector<bool> v)
{
    v.flip();
    return v;
}


vector<char> vector_char_plus_one(vector<char> v)
{
    for (auto& c : v)
        c++;
    return v;
}


vector<int8_t> vector_int8_plus_one(vector<int8_t> v)
{
    for (auto& c : v)
        c++;
    return v;
}


unordered_map<int, int> times2(unordered_map<int, int> m)
{
    for (auto& item : m)
        item.second *= item.first;
    return m;
}


tuple<int, int, int> times4(tuple<int, int, int> m)
{
    return make_tuple(get<0>(m) * 4, get<1>(m) * 4, get<2>(m) * 4);
}


std::expected<int, string> times5(int i)
{
    return i * 5;
}


variant<int, string> variant_int_and_string(variant<int, string> v)
{
    return std::visit(
        [](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>)
                return variant<int, string>(arg * 5);
            else
                return variant<int, string>(arg + arg);
        },
        v);
}


string_view stringview_identity(string_view s)
{
    return s;
}


int bool_arguments(bool b)
{
    return b ? 3 : 5;
}


bool bool_returns(int i)
{
    return i < 0;
}


int optional_arguments(optional<int> i)
{
    if (i)
        return *i;
    else
        return -123;
}


int optional_bool_arguments(optional<bool> b)
{
    if (!b)
        return -1;
    return *b ? 1 : 0;
}


optional<int> optional_returns(int i)
{
    if (i < 0)
        return nullopt;
    return i;
}


int gleam_option_arguments(gleam::option<int> i)
{
    if (i)
        return *i;
    return -123;
}


gleam::option<int> gleam_option_returns(int i)
{
    if (i < 0)
        return nullopt;
    return i;
}


auto gleam_tagged_image(int width, int height)
{
    return gleam::make_case<"image">(width, height);
}


using gleam_tagged_variant_result =
    variant<gleam::case_<"found", int, string>, gleam::case_<"missing", int>, gleam::case_<"empty">>;

static_assert(!is_yield_safe<int*>());
static_assert(!is_yield_safe<std::span<int>>());
static_assert(!is_yield_safe<ErlNifEnv*>());
static_assert(is_yield_safe<ErlNifPid>());
static_assert(is_yield_safe<gleam::option<int>>());
static_assert(!is_yield_safe<gleam::option<string_view>>());
static_assert(is_yield_safe<gleam::case_<"found", int, string>>());
static_assert(!is_yield_safe<gleam::case_<"found", int, string_view>>());
static_assert(is_yield_safe<gleam_tagged_variant_result>());
static_assert(is_yield_safe<std::expected<int, string>>());
static_assert(!is_yield_safe<std::expected<int, string_view>>());
static_assert(!is_yield_safe<std::expected<string_view, int>>());


gleam_tagged_variant_result gleam_tagged_variant(int id)
{
    if (id > 0)
        return gleam::make_case<"found">(id, string{"ok"});
    if (id < 0)
        return gleam::make_case<"missing">(id);
    return gleam::make_case<"empty">();
}


std::expected<int, int> expected_arguments(std::expected<int, int> result)
{
    if (result)
        return *result * 2;
    return std::unexpected(result.error() * 3);
}


std::expected<void, int> expected_void_negate(std::expected<void, int> result)
{
    if (result)
        return {};
    return std::unexpected(-result.error());
}


static_assert(is_yield_safe<std::expected<void, string>>());


// Gleam's Result(Nil, Int).
std::expected<std::monostate, int> expected_nil_negate(std::expected<std::monostate, int> result)
{
    if (result)
        return std::monostate{};
    return std::unexpected(-result.error());
}


static_assert(is_yield_safe<std::expected<std::monostate, string>>());


std::expected<int, int> get_expected(int i)
{
    if (i >= 0)
        return 123;
    else
        return std::unexpected(-123);
}


std::expected<int, std::string_view> get_expected_stringview_error(int i)
{
    if (i >= 0)
        return 123;
    else
        return std::unexpected("my bad...");
}


int atom_arguments(atom a)
{
    return a == "foo" ? 1 : -1;
}


atom atom_returns(int i)
{
    return i >= 0 ? "foo"_atom : "bar"_atom;
}


atom atom_identity(atom a)
{
    return a;
}


tuple<int, atom> tuple_with_invalid_atom()
{
    return make_tuple(1, "\xff"_atom);
}


std::pmr::vector<std::pmr::string> pmr_strings_identity(std::pmr::vector<std::pmr::string> v)
{
    return v;
}


vector<int> descending_keys(map<int, int, std::greater<>> m)
{
    vector<int> keys;
    for (const auto& [k, v] : m)
        keys.push_back(k);
    return keys;
}


yielding<pair<int, int>> simple_coroutine(int n)
{
    for (int i = 0; i < n; i++)
    {
        if (i < n - 1)
            co_yield nullopt;
        else
            co_yield make_pair(i, i * i);
    }
}


vector<vector<int>> nested_vector(vector<vector<int>> v)
{
    for (auto& inner : v)
        for (auto& x : inner)
            x *= 2;
    return v;
}


std::map<string, int> ordered_map_test(std::map<string, int> m)
{
    return m;
}


std::map<string, vector<int>> complex_nested_map(std::map<string, vector<int>> m)
{
    for (auto& [k, v] : m)
        for (auto& x : v)
            x *= 2;
    return m;
}


vector<std::byte> byte_vector_test(vector<std::byte> v)
{
    for (auto& b : v)
        b = std::byte{static_cast<unsigned char>(static_cast<unsigned char>(b) + 1)};
    return v;
}


struct MyResource
{
    int value;
};


resource<MyResource> make_resource(int i)
{
    return resource<MyResource>::alloc(i);
}


resource<MyResource> make_resource_incremented(int i)
{
    auto r = resource<MyResource>::alloc(i);
    r.get().value += 1;
    return r;
}


int use_resource(resource<MyResource> res)
{
    return res.get().value;
}


resource<MyResource> resource_identity(resource<MyResource> res)
{
    return res;
}


std::atomic<int> node_live_count = 0;


// Holds handles to its own type, so resource<Node> is used while Node is incomplete.
struct Node
{
    int value;
    vector<resource<Node>> children;

    Node(int value, vector<resource<Node>> children) :
        value(value),
        children(std::move(children))
    {
        node_live_count++;
    }

    ~Node()
    {
        node_live_count--;
    }
};


resource<Node> make_node(int value, vector<resource<Node>> children)
{
    return resource<Node>::alloc(value, std::move(children));
}


int tree_sum(resource<Node> node)
{
    int sum = node->value;
    for (const auto& child : node->children)
        sum += tree_sum(child);
    return sum;
}


void set_first_child(resource<Node> node, resource<Node> child)
{
    node->children.at(0) = child;
}


yielding<int> yield_node_value(resource<Node> node)
{
    co_yield nullopt;
    co_yield node->value;
}


int node_live()
{
    return node_live_count;
}


std::atomic<int> buffer_live_count = 0;


struct Buffer
{
    string bytes;

    explicit Buffer(string bytes) :
        bytes(std::move(bytes))
    {
        buffer_live_count++;
    }

    ~Buffer()
    {
        buffer_live_count--;
    }
};


// The returned binary alone keeps the buffer alive.
resource_binary<Buffer> buffer_tail(string bytes, size_t offset)
{
    if (offset > bytes.size())
        throw std::invalid_argument("offset out of range");
    auto buf = resource<Buffer>::alloc(std::move(bytes));
    return resource_binary{buf, buf->bytes.data() + offset, buf->bytes.size() - offset};
}


// Every chunk views, and keeps alive, the same buffer.
vector<resource_binary<Buffer>> buffer_chunks(string bytes, size_t chunk_size)
{
    if (chunk_size == 0)
        throw std::invalid_argument("chunk_size must be positive");
    auto buf = resource<Buffer>::alloc(std::move(bytes));
    vector<resource_binary<Buffer>> chunks;
    for (size_t i = 0; i < buf->bytes.size(); i += chunk_size)
        chunks.push_back(resource_binary{buf, buf->bytes.data() + i, std::min(chunk_size, buf->bytes.size() - i)});
    return chunks;
}


static_assert(requires(ErlNifEnv* env, const map<int, resource_binary<Buffer>>& m) {
    type_cast<map<int, resource_binary<Buffer>>>::to_term(env, m);
});


int buffer_live()
{
    return buffer_live_count;
}


// Never defined: handles to an opaque type can still be copied, moved, assigned, released,
// and encoded. Compile-only.
struct Opaque;

ERL_NIF_TERM opaque_handle_ops(ErlNifEnv* env, resource<Opaque> a, resource<Opaque>& b)
{
    auto copy = a;
    b = std::move(copy);
    return type_cast<resource<Opaque>>::to_term(env, b);
}


int throwing_resource_destructions = 0;


struct ThrowingResource
{
    ThrowingResource(int)
    {
        throw std::runtime_error("constructor failed");
    }

    ~ThrowingResource()
    {
        throwing_resource_destructions++;
    }
};


// Returns how often ~ThrowingResource ran, which must be 0 since construction never succeeds.
int make_throwing_resource()
{
    try
    {
        resource<ThrowingResource>::alloc(1);
    }
    catch (const std::runtime_error&)
    {}
    return throwing_resource_destructions;
}


int throw_error(int i)
{
    if (i == 0)
        throw erl_error<string>("some error");
    if (i == 1)
        throw erl_error<int>(42);
    if (i == 2)
        throw erl_error<atom>("\xff"_atom);
    return i;
}


int dirty_cpu_test(int i)
{
    return i * 10;
}


binary binary_identity(binary b)
{
    return b;
}


void raise_runtime_error_test()
{
    throw std::runtime_error("this is a runtime error from C++");
}


term term_identity(term t)
{
    return t;
}


ErlNifPid pid_identity(ErlNifPid pid)
{
    return pid;
}


bool process_alive(ErlNifEnv* env, ErlNifPid pid)
{
    return enif_is_process_alive(env, &pid);
}


// Sends from the NIF's own thread, then from a thread the VM didn't create.
pair<bool, bool> send_twice(ErlNifEnv* env, ErlNifPid pid, term msg)
{
    const bool from_nif = expp::send(env, pid, pair{"from_nif"_atom, msg});
    bool from_thread = false;
    std::thread([&] { from_thread = expp::send(nullptr, pid, pair{"from_thread"_atom, msg}); }).join();
    return {from_nif, from_thread};
}


int8_t int8_identity(int8_t x)
{
    return x;
}


uint8_t uint8_identity(uint8_t x)
{
    return x;
}


char char_identity(char x)
{
    return x;
}


int16_t int16_identity(int16_t x)
{
    return x;
}


float float_identity(float x)
{
    return x;
}


multimap<string, int> multimap_test(multimap<string, int> m)
{
    return m;
}


unordered_multimap<string, int> unordered_multimap_test(unordered_multimap<string, int> m)
{
    return m;
}


int int_identity(int x)
{
    return x;
}


uint32_t uint32_identity(uint32_t x)
{
    return x;
}


int64_t int64_identity(int64_t x)
{
    return x;
}


uint64_t uint64_identity(uint64_t x)
{
    return x;
}


double double_identity(double x)
{
    return x;
}


std::string string_identity(std::string s)
{
    return s;
}


int dirty_io_test(int i)
{
    return i * 100;
}


int named_nif_impl(int x)
{
    return x + 1;
}


yielding<int> yield_values(int n)
{
    for (int i = 0; i < n; i++)
    {
        if (i < n - 1)
            co_yield nullopt;
        else
            co_yield i * 10;
    }
}


// Returns the scheduler type (enif_thread_type) each step ran on.
yielding<vector<int>> schedule_on_steps()
{
    vector<int> thread_types{enif_thread_type()};
    co_yield schedule_on(DirtyFlags::DirtyCpu);
    thread_types.push_back(enif_thread_type());
    co_yield schedule_on(DirtyFlags::NotDirty);
    thread_types.push_back(enif_thread_type());
    co_yield thread_types;
}


yielding<int> yield_exhaustion()
{
    co_return;
}


yielding<int> yield_invalid_argument(int n)
{
    co_yield nullopt;
    if (n < 0)
        throw std::invalid_argument("n must be >= 0");
    co_yield n;
}


yielding<int> yield_unknown_exception()
{
    co_yield nullopt;
    throw 42;
}

struct MyPersistentType
{
    int value;
    vector<int> data;
};


template <>
struct expp::is_yield_persistent<MyPersistentType> : std::true_type
{};


template <>
struct expp::type_cast<MyPersistentType>
{
    static ERL_NIF_TERM to_term(ErlNifEnv* env, const MyPersistentType& t)
    {
        return enif_make_tuple2(env, enif_make_int(env, t.value), type_cast<vector<int>>::to_term(env, t.data));
    }

    static MyPersistentType from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        const ERL_NIF_TERM* tup_array;
        int arity;
        if (!enif_get_tuple(env, term, &arity, &tup_array) || arity != 2)
            throw std::invalid_argument("expected {int, list}");
        return MyPersistentType{
            type_cast<int>::from_term(env, tup_array[0]), type_cast<vector<int>>::from_term(env, tup_array[1])
        };
    }
};


yielding<MyPersistentType> yield_persistent_type(int n)
{
    for (int i = 0; i < n; i++)
    {
        if (i < n - 1)
            co_yield std::nullopt;
        else
            co_yield MyPersistentType{i, {1, 2, 3}};
    }
}


int load_time_value = 0;


// Resource types are open by the time the user's load callback runs.
int load(ErlNifEnv*, void**, ERL_NIF_TERM)
{
    load_time_value = resource<MyResource>::alloc(42)->value;
    return 0;
}


int loaded_value()
{
    return load_time_value;
}


// Compile-only: LOAD may be any null value.
using load_callback = int (*)(ErlNifEnv*, void**, ERL_NIF_TERM);
constexpr load_callback null_load = nullptr;
[[maybe_unused]] constexpr load_callback null_loads[] = {
    expp::detail::on_load<nullptr>, expp::detail::on_load<NULL>, expp::detail::on_load<null_load>
};


MODULE(
    Elixir.MyMod,
    load,
    nullptr,
    def(vector_times_int, DirtyFlags::NotDirty),
    def(bool_vector_negate, DirtyFlags::NotDirty),
    def(vector_char_plus_one, DirtyFlags::NotDirty),
    def(vector_int8_plus_one, DirtyFlags::NotDirty),
    def(times2, DirtyFlags::NotDirty),
    def(times4, DirtyFlags::NotDirty),
    def(times5, DirtyFlags::NotDirty),
    def(stringview_identity, DirtyFlags::NotDirty),
    def(variant_int_and_string, DirtyFlags::NotDirty),
    def(bool_arguments, DirtyFlags::NotDirty),
    def(bool_returns, DirtyFlags::NotDirty),
    def(optional_arguments, DirtyFlags::NotDirty),
    def(optional_bool_arguments, DirtyFlags::NotDirty),
    def(optional_returns, DirtyFlags::NotDirty),
    def(gleam_option_arguments, DirtyFlags::NotDirty),
    def(gleam_option_returns, DirtyFlags::NotDirty),
    def(gleam_tagged_image, DirtyFlags::NotDirty),
    def(gleam_tagged_variant, DirtyFlags::NotDirty),
    def(expected_arguments, DirtyFlags::NotDirty),
    def(get_expected, DirtyFlags::NotDirty),
    def(expected_void_negate, DirtyFlags::NotDirty),
    def(expected_nil_negate, DirtyFlags::NotDirty),
    def(get_expected_stringview_error, DirtyFlags::NotDirty),
    def(atom_arguments, DirtyFlags::NotDirty),
    def(atom_returns, DirtyFlags::NotDirty),
    def(atom_identity, DirtyFlags::NotDirty),
    def(tuple_with_invalid_atom, DirtyFlags::NotDirty),
    def(pmr_strings_identity, DirtyFlags::NotDirty),
    def(descending_keys, DirtyFlags::NotDirty),
    def(simple_coroutine, DirtyFlags::NotDirty),
    def(nested_vector, DirtyFlags::NotDirty),
    def(ordered_map_test, DirtyFlags::NotDirty),
    def(complex_nested_map, DirtyFlags::NotDirty),
    def(byte_vector_test, DirtyFlags::NotDirty),
    def(make_resource, DirtyFlags::NotDirty),
    def(make_resource_incremented, DirtyFlags::NotDirty),
    def(use_resource, DirtyFlags::NotDirty),
    def(resource_identity, DirtyFlags::NotDirty),
    def(make_node, DirtyFlags::NotDirty),
    def(tree_sum, DirtyFlags::NotDirty),
    def(set_first_child, DirtyFlags::NotDirty),
    def(yield_node_value, DirtyFlags::NotDirty),
    def(node_live, DirtyFlags::NotDirty),
    def(buffer_tail, DirtyFlags::NotDirty),
    def(buffer_chunks, DirtyFlags::NotDirty),
    def(buffer_live, DirtyFlags::NotDirty),
    def(make_throwing_resource, DirtyFlags::NotDirty),
    def(loaded_value, DirtyFlags::NotDirty),
    def(throw_error, DirtyFlags::NotDirty),
    def(dirty_cpu_test, DirtyFlags::DirtyCpu),
    def(binary_identity, DirtyFlags::NotDirty),
    def(term_identity, DirtyFlags::NotDirty),
    def(pid_identity, DirtyFlags::NotDirty),
    def(process_alive, DirtyFlags::NotDirty),
    def(send_twice, DirtyFlags::NotDirty),
    def(raise_runtime_error_test, DirtyFlags::NotDirty),
    def(int8_identity, DirtyFlags::NotDirty),
    def(uint8_identity, DirtyFlags::NotDirty),
    def(char_identity, DirtyFlags::NotDirty),
    def(int16_identity, DirtyFlags::NotDirty),
    def(float_identity, DirtyFlags::NotDirty),
    def(multimap_test, DirtyFlags::NotDirty),
    def(unordered_multimap_test, DirtyFlags::NotDirty),
    def(int_identity, DirtyFlags::NotDirty),
    def(uint32_identity, DirtyFlags::NotDirty),
    def(int64_identity, DirtyFlags::NotDirty),
    def(uint64_identity, DirtyFlags::NotDirty),
    def(double_identity, DirtyFlags::NotDirty),
    def(string_identity, DirtyFlags::NotDirty),
    def(dirty_io_test, DirtyFlags::DirtyIO),
    def(named_nif_impl, "named_nif", DirtyFlags::NotDirty),
    def(yield_values, DirtyFlags::NotDirty),
    def(schedule_on_steps, DirtyFlags::NotDirty),
    def(schedule_on_steps, "schedule_on_steps_dirty", DirtyFlags::DirtyCpu),
    def(yield_exhaustion, DirtyFlags::NotDirty),
    def(yield_invalid_argument, DirtyFlags::NotDirty),
    def(yield_unknown_exception, DirtyFlags::NotDirty),

    def(yield_persistent_type, DirtyFlags::NotDirty), )
