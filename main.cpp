#include "Applications.h"

int main()
{
    Applications app;

    if (!app.Initialize())
        return -1;

    app.Run();

    return 0;
}