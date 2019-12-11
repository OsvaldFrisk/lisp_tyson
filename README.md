# lisp_tyson

Tyson is a lisp language implemented entirely in C. I recently did course at university in programming paradigms, in which I programmed my first lisp program. Having enjoyed the experience and yearning for a chance to write serious program in C, I found the book [`Build Your Own Lisp`](http://www.buildyourownlisp.com/). This is also where the inspiration of the name for the language comes from, in the fourth subsection of the first chapter, [`Why build a Lisp`](http://www.buildyourownlisp.com/chapter1_introduction#why_build_a_lisp), the author lightens up the mood by posting the following picture.

![Lisp_user](https://github.com/OsvaldFrisk/lisp_tyson/blob/master/documentation/typical_lisp_user.png)


### Prerequisites


To run Tyson you need a compiled version of the C code and thus you need a C compiler. In my case I used the default compiler for Mac OS, which is the Clang C compiler.

Running the following command in the terminal should result in an executable named _tyson_.

```
cc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o tyson
```

## Editor and Tyson

For ease of writing in tyson, I have made a minimalistic visual studio code extension for syntax highlighting. To use the extension copy/move the folder `tyson_extension` into the `<user home>/.vscode/extensions` folder and restart Visual Studio Code.

in Bash on Mac OS:
```
cp -r tyson_extension/ ~/.vscode/extensions/tyson_extension
```

If you followed the instructions and they all worked, opening the program `./programs/fib.ty` in vscode should look like so.

![vsc_fib](https://github.com/OsvaldFrisk/lisp_tyson/blob/master/documentation/vsc_fib.png)

And finally running the program should look like so:
```
$./tyson ./programs/fib.ty 
$"Calculating Fibonacci for 20..." 
$6765 
```

## Acknowledgments

* Hat tip to anyone whose code was used
* Inspiration
* etc