#include "ns3/core-module.h"

using namespace ns3;
using namespace std;

static double
CbOne(double a, double b)
{
    cout<<" call back a="<<a<<", b="<<b<<endl;
    return 0.0;
}

int main()
{
    Callback<double, double, double> one;
    one = MakeCallback(&CbOne);
    NS_ASSERT(!one.IsNull());
    double retone;
    retone = one(10.0, 20.0);

    cout<<"retone is: "<<retone<<endl;
}
