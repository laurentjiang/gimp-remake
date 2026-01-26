/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "GIMP Remake", "index.html", [
    [ "Error Handling Architecture", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html", [
      [ "Overview", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md1", null ],
      [ "Design Goals", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md2", null ],
      [ "Architecture Components", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md3", [
        [ "1. Error Codes (<span class=\"tt\">error_codes.h</span>)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md4", [
          [ "Error Categories", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md5", null ],
          [ "Error Severity", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md6", null ]
        ] ],
        [ "2. Error Information (<span class=\"tt\">error_result.h</span>)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md7", [
          [ "ErrorInfo Class", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md8", null ],
          [ "Result&lt;T&gt; Type", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md9", null ]
        ] ],
        [ "3. Error Handler (<span class=\"tt\">error_handler.h/cpp</span>)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md10", [
          [ "Key Responsibilities", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md11", null ],
          [ "Error Flow", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md12", null ],
          [ "Recovery Mechanism", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md13", null ],
          [ "Crash Handling", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md14", null ]
        ] ],
        [ "4. Exception Classes (<span class=\"tt\">exceptions.h</span>)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md15", [
          [ "Exception Hierarchy", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md16", null ],
          [ "Static Factory Methods", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md17", null ]
        ] ],
        [ "5. Error Context (<span class=\"tt\">error_handler.h</span>)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md18", null ]
      ] ],
      [ "Error Handling Strategies", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md19", [
        [ "Strategy 1: Result&lt;T&gt; (Recommended for Expected Errors)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md20", null ],
        [ "Strategy 2: Exceptions (For Unexpected Errors)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md21", null ],
        [ "Strategy 3: Direct Error Handler (For Logging)", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md22", null ]
      ] ],
      [ "Integration Points", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md23", [
        [ "Application Startup", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md24", null ],
        [ "Subsystem Integration", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md25", null ]
      ] ],
      [ "Testing Strategy", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md26", [
        [ "Unit Testing", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md27", null ],
        [ "Integration Testing", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md28", null ]
      ] ],
      [ "Performance Considerations", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md29", null ],
      [ "Future Enhancements", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md30", null ],
      [ "Summary", "md_docs_2ERROR__HANDLING__ARCHITECTURE.html#autotoc_md31", null ]
    ] ],
    [ "Error Handling Guide", "md_docs_2ERROR__HANDLING__GUIDE.html", [
      [ "Table of Contents", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md33", null ],
      [ "Quick Start", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md34", [
        [ "Include Headers", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md35", null ],
        [ "Initialize Error Handler (in main)", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md36", null ]
      ] ],
      [ "Choosing an Error Strategy", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md37", [
        [ "Use Result&lt;T&gt; when:", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md38", null ],
        [ "Use Exceptions when:", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md39", null ],
        [ "Use Direct Reporting when:", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md40", null ]
      ] ],
      [ "Using Result&lt;T&gt;", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md41", [
        [ "Basic Usage", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md42", null ],
        [ "Result&lt;T&gt; with Custom Context", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md43", null ],
        [ "Result&lt;void&gt; for Operations Without Return Value", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md44", null ],
        [ "Chaining with Map", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md45", null ],
        [ "Using ValueOr for Defaults", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md46", null ]
      ] ],
      [ "Using Exceptions", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md47", [
        [ "Throwing Exceptions", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md48", null ],
        [ "Catching Specific Exceptions", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md49", null ],
        [ "Catching All GIMP Exceptions", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md50", null ],
        [ "Using HANDLE_EXCEPTIONS Macro", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md51", null ]
      ] ],
      [ "Error Reporting", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md52", [
        [ "Direct Error Reporting", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md53", null ],
        [ "Using the REPORT_ERROR Macro (with context)", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md54", null ],
        [ "Reporting Fatal Errors", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md55", null ]
      ] ],
      [ "Recovery Handlers", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md56", [
        [ "Registering a Recovery Handler", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md57", null ],
        [ "Recovery for I/O Errors", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md58", null ],
        [ "Recovery for Memory Errors", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md59", null ]
      ] ],
      [ "Error Context", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md60", [
        [ "Adding Context to Function Calls", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md61", null ],
        [ "Nested Context", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md62", null ]
      ] ],
      [ "Best Practices", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md63", [
        [ "1. Always Check Results", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md64", null ],
        [ "2. Provide Meaningful Context", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md65", null ],
        [ "3. Use Appropriate Severity", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md66", null ],
        [ "4. Don't Catch and Ignore", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md67", null ],
        [ "5. Initialize Early, Shutdown Late", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md68", null ]
      ] ],
      [ "Common Patterns", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md69", [
        [ "Pattern 1: File I/O with Result", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md70", null ],
        [ "Pattern 2: Resource Allocation with Exceptions", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md71", null ],
        [ "Pattern 3: Validation with Early Return", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md72", null ],
        [ "Pattern 4: Batch Operations", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md73", null ],
        [ "Pattern 5: Callback-Based Error Notification", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md74", null ]
      ] ],
      [ "Testing Error Handling", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md75", [
        [ "Testing Result&lt;T&gt;", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md76", null ],
        [ "Testing Exceptions", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md77", null ],
        [ "Testing Recovery", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md78", null ]
      ] ],
      [ "Summary Checklist", "md_docs_2ERROR__HANDLING__GUIDE.html#autotoc_md79", null ]
    ] ],
    [ "Error Handling Quick Reference", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html", [
      [ "Headers to Include", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md81", null ],
      [ "Error Categories", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md82", null ],
      [ "Common Error Codes", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md83", null ],
      [ "Using Result&lt;T&gt;", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md84", [
        [ "Return a Result", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md85", null ],
        [ "Check and Use Result", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md86", null ],
        [ "Result&lt;void&gt;", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md87", null ]
      ] ],
      [ "Using Exceptions", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md88", [
        [ "Throw an Exception", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md89", null ],
        [ "Catch Exceptions", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md90", null ],
        [ "Available Exception Types", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md91", null ]
      ] ],
      [ "Error Reporting", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md92", [
        [ "Report Error", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md93", null ],
        [ "Report with Macro (includes context)", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md94", null ],
        [ "Fatal Error", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md95", null ]
      ] ],
      [ "Error Context", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md96", [
        [ "Add Context to Function", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md97", null ],
        [ "Nested Context", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md98", null ]
      ] ],
      [ "Recovery Handlers", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md99", [
        [ "Register Recovery Handler", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md100", null ]
      ] ],
      [ "Error Callbacks", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md101", [
        [ "Register Callback", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md102", null ],
        [ "Category-Specific Callback", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md103", null ]
      ] ],
      [ "Initialization", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md104", [
        [ "In main()", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md105", null ]
      ] ],
      [ "Configuration", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md106", null ],
      [ "Testing", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md107", null ],
      [ "When to Use What", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md108", null ],
      [ "Common Patterns", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md109", [
        [ "File I/O", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md110", null ],
        [ "Validation", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md111", null ],
        [ "Resource Allocation", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md112", null ]
      ] ],
      [ "Error Severity Levels", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md113", null ],
      [ "See Also", "md_docs_2ERROR__HANDLING__QUICK__REFERENCE.html#autotoc_md114", null ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"add__layer__command_8cpp.html",
"files.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';