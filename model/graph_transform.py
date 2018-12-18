import sys

import tensorflow as tf

input_path = sys.argv[1]
output_path = sys.argv[2]

with open(input_path, "rb") as f:
    graph_def = tf.GraphDef()
    graph_def.ParseFromString(f.read())

tf.import_graph_def(graph_def, name="")
graph = tf.get_default_graph()

output_graph_def = tf.GraphDef()
for node in graph_def.node:
    replace_node = tf.NodeDef()
    replace_node.CopyFrom(node)
    if node.name == "value":
        continue
    if node.name == "zero/value_head/Reshape_1":
        replace_node.name = "value"
    if node.name == "inputs":
        replace_node.attr["dtype"].CopyFrom(tf.AttrValue(type=tf.float32.as_datatype_enum))
    for i, inp in enumerate(node.input):
        if inp == "Cast":
            replace_node.input[i] = "inputs"
    output_graph_def.node.extend([replace_node])

with open(output_path, "wb") as f:
    f.write(output_graph_def.SerializeToString())
