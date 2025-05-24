import torch
import matplotlib.pyplot as plt
from train import DQN, Game2048, visualize_board

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# 初始化模型
input_size = 17
output_size = 4
model = DQN(input_size, output_size).to(device)
model.load_state_dict(torch.load('2048_dqn_model.pth', map_location=device))
model.eval()

# 初始化环境
env = Game2048()
state = env.reset()
done = False

fig, ax = plt.subplots()
total_reward = 0

max_steps = 1000  # 防止死循环
steps = 0

while not done and steps < max_steps:
    visualize_board(env.board, ax)
    state_tensor = torch.FloatTensor(state).unsqueeze(0).to(device)
    with torch.no_grad():
        q_values = model(state_tensor)
        action = torch.argmax(q_values, dim=1).item()
    next_state, reward, done = env.step(action)
    state = next_state
    total_reward += reward
    steps += 1

plt.close()
print("游戏结束，总得分：", total_reward)
print("最大方块：", int(state[-1]))