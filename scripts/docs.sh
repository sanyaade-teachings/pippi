#!/usr/bin/bash
echo ":: Tangling tutorials..."
for tut in docs/tutorials/*.pmd;
do
    ptangle $tut
done

echo ":: Executing tutorials..."
for tut in docs/tutorials/*.py;
do
    python $tut
done

echo ":: Weaving tutorials..."
for tut in docs/tutorials/*.pmd;
do
    pweave -f pandoc $tut
done

echo ":: Encoding renders..."
for wav in docs/tutorials/renders/*.wav;
do
    sox -S "$wav" "${wav%.wav}.ogg";
done
for flac in docs/tutorials/renders/*.flac;
do
    sox -S "$flac" "${flac%.flac}.ogg";
done

echo ":: Generating HTML..."
portray as_html --overwrite -o pippi.world

echo ":: Updating permissions..."
chmod -R 755 pippi.world

echo ":: Done!"
