#include <functional>
#include <iostream>
#include <vector>

auto puncture(std::size_t num_bits, std::vector<int> P, std::function<int(int)> i_from_j) -> std::vector<int> {
    auto t = P.size() - 1;

    auto period = 8;

    std::vector<int> res(num_bits * 4, 0);

    for (auto j = 1; j <= res.size(); j++) {
        auto i = i_from_j(j);
        auto k = period * ((i - 1) / t) + P[i - t * ((i - 1) / t)];
        if (k - 1 < res.size())
            res[k - 1] = j;
    }

    return res;
}

auto print(std::vector<int> const& res) -> void {
    for (auto i = 0; i < res.size();) {
        auto j = i;
        std::cout << "  ";
        for (; j < i + 16; j++) {
            std::cout << res[j] << " ";
        }
        std::cout << std::endl;
        i = j;
    }
}

auto main(int argc, char** argv) -> int {
    auto rate_2_3 = puncture(4, {0, 1, 2, 5}, [](int j) { return j; });
    auto rate_1_3 = puncture(4, {0, 1, 2, 3, 5, 6, 7}, [](int j) { return j; });
    auto rate_292_432 = puncture(11 * 4, {0, 1, 2, 5}, [](int j) { return j + (j - 1) / 65; });
    auto rate_148_432 = puncture(3 * 4, {0, 1, 2, 3, 5, 6, 7}, [](int j) { return j + (j - 1) / 35; });

    std::cout << "Rate 2/3:" << std::endl;
    print(rate_2_3);

    std::cout << "Rate 1/3:" << std::endl;
    print(rate_1_3);

    std::cout << "Rate 292/432:" << std::endl;
    print(rate_292_432);

    std::cout << "Rate 148/432:" << std::endl;
    print(rate_148_432);
}
