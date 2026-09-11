#include "Applications.h"

//black hole
int main()
{
    Applications app;

    if (!app.Initialize())
        return -1;

    app.Run();

    return 0;
}
