# flashcards

![](https://lambdafun.neocities.org/assets/cards.gif)

## Online resources (older->newer)

- https://soykaf.me/2019-03-28-exg.html
- https://soykaf.me/2019-04-05-45Y.html
- https://soykaf.me/assets/cards2.mp4

## Build and run

    make
    ./a.out -f hiragana.csv
    # at some point enter !save and exit with CTRL-D

## Tutorial

Edit `cards.csv` and save your session by typing `!save` while studying.

Here is a typical `cards.csv`:

    front1,back1,
    front2,back2,
    ...

You can also use the `-f` flag to use another file such as hiragana.csv 
for instance.

WARNING! This tool won't care about the order in which cards were 
written in the file!

If you want `cards.c` to know which cards to show first, you'll have to 
fill the last field of each line with a number. The higher it is the 
earlier the card will appear. This number is altered by `cards.c` when 
you type `!save`.

If you want your cards to appear in the same order as in cards.csv, run 
this:

    cat cards.csv | awk 'BEGIN{i=1000}{print $0 i--}' > cards2.csv

## Tips

    cat hiragana.csv katakana.csv > cards.csv
    ./a.out

## Credits

This project includes files from csv_parser 

https://github.com/semitrivial/csv_parser

## Development notes

Known to compile and run on Debian Stretch and FreeBSD 12
