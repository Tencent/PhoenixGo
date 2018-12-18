#!/usr/bin/env python

import sys

import tensorflow as tf

input_path = sys.argv[1]

with tf.Session() as session:
    with open(input_path, "rb") as f:
        graph_def = tf.GraphDef()
        graph_def.ParseFromString(f.read())
    tf.import_graph_def(graph_def, name="")
    global_step = session.run("global_step:0")
    print global_step
