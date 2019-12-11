# lisp_tyson

Tyson is a lisp language implemented entirely in C. I recently did course at university in programming paradigms, in which I programmed my first lisp program. Having enjoyed the experience and yearning for a chance to write serious program in C, I found the book [`Build Your Own Lisp`](http://www.buildyourownlisp.com/). This is also where the inspiration of the name for the language comes from, in the fourth subsection of the first chapter, [`Why build a Lisp `](http://www.buildyourownlisp.com/chapter1_introduction#why_build_a_lisp), the author lightens up the mood by posting the following picture.

![Lisp_user](https://github.com/OsvaldFrisk/lisp_tyson/blob/master/documentation/typical_lisp_user.png)


### Prerequisites


To run the Tyson you need a compiled version fo the C code, thus you need a C compiler. In my case I used the default compiler for Mac OS, which is the Clang C compiler.

Running the following command in the terminal should result in an executable named _tyson_.

```
cc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o tyson
```

## Editor and Tyson

For ease of writing in tyson, I have made a minimalistic visual studio code extension for syntax highlighting. 


```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone whose code was used
* Inspiration
* etc