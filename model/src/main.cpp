#include "dbn_t.hpp"

struct net_t
{
    template<int first_input_row>
    using bp_type = bp<double, 1, nadam, ReLu, XavierGaussian, first_input_row, 200, 200, 200>;
    using dbn_type = dbn_t<bp_type, double, 200, 300, 400>;
};

int main(int argc, char* argv[])
{
    net_t nt;
    return 0;
}