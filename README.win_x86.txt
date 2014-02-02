This is a build of version 0.5 of lib_mysqludf_str for 32-bit MySQL 5.1+ and
Windows XP SP3 or later. It is licensed under the terms of the LGPL
version 2.1 or later, the exact text of which is located in COPYING.

All source code for this project is downloadable from GitHub:
https://github.com/mysqludf/lib_mysqludf_str/


INSTALLATION
================================================================================
To install the lib_mysqludf_str UDFs:

 1. Make sure that you have installed the 32-bit build of MySQL 5.1 or later.

    Note: The lib_mysqludf_str.dll binary has only been tested with MySQL version
    5.6, but should work with MySQL 5.5, 5.1 and 5.0 as well. It is, however,
    incompatible with MySQL 3.23/4.0/4.1.

 2. Look for a file named msvcr110.dll in your System32 folder (on a 64-bit Windows
    machine, look for this file in the SysWOW64 folder instead). If this file is
    not present, then install the latest Visual C++ Redistributable for Visual Studio 2012
    vcredist_x86.exe:
    http://www.microsoft.com/en-us/download/details.aspx?id=30679

 3. Copy lib_mysqludf_str.dll to the MySQL installation's lib\plugin folder.

    With a default installation, this is C:\Program Files\MySQL\MySQL Server 5.6\lib\plugin\

    Note: You do NOT need to re-start MySQL after copying lib_mysqludf_str.dll
    into the plugin directory.

 4. As root, source installdb.win.sql or simply paste the contents into the CLI client.

 5. Test the setup by executing:
    SELECT lib_mysqludf_str_info() FROM DUAL;

    The result should be:
    +------------------------------+
    | lib_mysqludf_str_info()      |
    +------------------------------+
    | lib_mysqludf_str version 0.5 |
    +------------------------------+


TROUBLESHOOTING
================================================================================
  * ERROR 1126 (HY000): Can't open shared library 'lib_mysqludf_str' (errno: 0 )

    This either means that the Visual C++ Redistributable for Visual Studio 2012
    is not installed or that MySQL could not find lib_mysqludf_str.dll in the
    plugin directory.

    Look for a file named msvcr110.dll in your System32 folder. If this file is
    not present, then install the latest Visual C++ Redistributable for Visual Studio 2012
    vcredist_x86.exe.

    Verify that lib_mysqludf_str.dll was copied into the plugin directory by
    executing:
    SHOW VARIABLES LIKE 'plugin_dir';

    The result should be:
    +---------------+-----------------------------------------------------+
    | Variable_name | Value                                               |
    +---------------+-----------------------------------------------------+
    | plugin_dir    | C:\Program Files\MySQL\MySQL Server 5.6\lib\plugin\ |
    +---------------+-----------------------------------------------------+

    If it's not, then copy lib_mysqludf_str.dll to the listed directory.

  * ERROR 1126 (HY000): Can't open shared library 'lib_mysqludf_str.dll' (errno: 126 )

    This error can happen when you don't have the 32-bit Visual C++ Redistributable for Visual Studio 2012
    installed.

    On a 64-bit Windows machine, look for a file named msvcr110.dll in the
    SysWOW64 folder. You need to install vcredist_x86.exe from http://www.microsoft.com/en-us/download/details.aspx?id=30679

  * ERROR 1126 (HY000): Can't open shared library 'lib_mysqludf_str.dll' (errno: 193)

    This error can happen when you try to use the 32-bit version of lib_mysqludf_str
    with a 64-bit MySQL server. Either use the 64-bit version of lib_mysqludf_str
    or install 32-bit MySQL.

  * ERROR 1127 (HY000): Can't find symbol in library

    This means that MySQL was able to find lib_mysqludf_str.dll, but for some
    reason, it could not "see" the UDF in the DLL module.

    If you are typing the CREATE FUNCTION statements manually, make sure that
    you have typed the UDF name exactly as listed in installdb.win.sql. For example,
    instead of "STR_XOR", you must type "str_xor" (all lowercase).


If you encounter any other problem, please feel free to contact me at:
    dtrebbien@gmail.com
