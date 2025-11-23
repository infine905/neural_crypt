import torch
import torch.nn as nn

INPUT_SIZE = 32 + 64
HIDDEN_SIZE = 64
OUTPUT_SIZE = 32

class CryptoNet(nn.Module):
    def __init__(self):
        super(CryptoNet, self).__init__()

        self.relu = nn.ReLU()

        # Fully connected layer 1 (weight matrix W1 and bias matrix B1)
        self.fc1 = nn.Linear(INPUT_SIZE, HIDDEN_SIZE)

        # Fully connected layer 2 (weight matrix W2 and bias matrix B2)
        self.fc2 = nn.Linear(HIDDEN_SIZE, OUTPUT_SIZE)

    def forward(self, x):
        out = self.fc1(x)
        out = self.relu(out)
        out = self.fc2(out)
        return out

# 2. Initialization and "Training"
# We use the Kaiming/Xavier initialization, which ensures good signal propagation.
model = CryptoNet()

# For this task, high-quality random initialization works P.
def init_weights(m):
    if isinstance(m, nn.Linear):
        torch.nn.init.xavier_uniform_(m.weight)
        m.bias.data.fill_(0.01)

model.apply(init_weights)

print("NeuralNet Initialized. Generating C++ header...")

# 3. Export weights to hpp file
def export_tensor_to_cpp(name, tensor):
    data = tensor.detach().numpy().flatten()
    cpp_str = f"\tconstexpr inline float {name}[{len(data)}] = {{\n\t\t"
    for i, val in enumerate(data):
        cpp_str += f"{val:.6f}f, "
        if (i + 1) % 8 == 0:
            cpp_str += "\n\t\t"
    cpp_str = cpp_str.rstrip(", \n\t\t") + "\n\t};"
    return cpp_str

with open("weights.h", "w") as f:
    f.write("#ifndef WEIGHTS_H\n#define WEIGHTS_H\n\n")
    f.write("// Automatically generated weights\n\n")

    f.write("namespace weights\n{\n")
    
    # Layer 1
    f.write(f"\t// Input -> Hidden (Weights: {HIDDEN_SIZE}x{INPUT_SIZE})\n")
    f.write(export_tensor_to_cpp("W1", model.fc1.weight))
    f.write(f"\n\t// Hidden Biases ({HIDDEN_SIZE})\n")
    f.write(export_tensor_to_cpp("B1", model.fc1.bias))

    # Layer 2
    f.write(f"\n\t// Hidden -> Output (Weights: {OUTPUT_SIZE}x{HIDDEN_SIZE})\n")
    f.write(export_tensor_to_cpp("W2", model.fc2.weight))
    f.write(f"\n\t// Output Biases ({OUTPUT_SIZE})\n")
    f.write(export_tensor_to_cpp("B2", model.fc2.bias))
    
    f.write("\n}\n")

    f.write("\n#endif\n")

print("Done! The 'weights.h' file has been created.")