#!/usr/local/bin/python

import analysis
import os.path


def main():
    root_dir = '/Users/reuben/development/waveguide/build/utils/siltanen2013'

    fnames = [os.path.join(root_dir, i) for i in [
        'omnidirectional.exact_img_src.wav',
        'omnidirectional.waveguide.wav',
        #'no_processing.exact_img_src.wav',
        #'no_processing.waveguide.wav',
    ]]

    max_frequency = 120
    room_dim = [5.56, 3.97, 2.81]

    analysis.modal_analysis(fnames, max_frequency, room_dim)

if __name__ == '__main__':
    main()
