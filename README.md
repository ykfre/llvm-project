# Changes for supporting c++ excpetions not using directly the seh mechainsm in windows.
* Disabling inlining by force.
* Change _CxxThrowException and _CxxFrameHandler3 to MyCxxThrowException and MyCxxFrameHandler3 (usefull for making it possilbe to use another Crts) .
* #### Add a refactoring tool helper - for making it simpler to move from c/ c++ which not throws exceptions to throw c++ exception by adding a noexcept keyword to every function declaration (except extern "C" functions), and then we will get a warning if this function tries to call a function which may raise an exception, so it will be safe always to change a function from state of not throwing, to a state of throwing.

## Added Warnings:
1) No empty throw - meaning rethrow exception.
    ##### for example:
        try
        {
            throw 1;
        }
        catch(...)
        {
            throw;
        }
    will raise an error in compilation time.

2) No catching exception not by refernce, including but not limitied to pointer passing.
    ##### for example:
        try
        {
            throw 1;
        }
        catch(int a)
        {
        }
    will raise an error in compilation time.

3) No nested try catch in the same function.
    ##### for example:
        class m{};
        try
        {
            try
            {
                throw m;
            }
            catch(int a)
            {
            }
         }
        catch(...)
        {
        }
    will raise an error in compilation time.

4) Make sure a noexcept function can not call a function which may throw without the function to be placed under an ellipsis try catch block
    ##### for example:
        void f()
        {
            throw 1;
        }

        void g() noexcpet()
        {
            f();
        }

    ##### you need to replace it to:
        void f()
        {
            throw 1;
        }

        void g() noexcpet()
        {
            try
            {
                f();
            }
            catch(const int& a)
            {
            //  logic
            }
            // For now you must use elipsis (even if you do nothing in this catch), and not catch it by int, although f only throws int
            // To make sure the error cann't escape the current function, in the futre I may add an option to catch the exceptions by a 
            // base class exception.
            catch(...)
            {
                // empty on purpose.
            }
        }
