# import pandas as pd
# import matplotlib.pyplot as plt

# data = pd.read_csv('fashion_data.csv')

# data = data.dropna(axis=0)
# print(data.isnull().sum())

# data.set_index('id', inplace=True)
# print(data.head())

import torch
x = torch.rand(5, 3)
print(x)