import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv('fashion_data.csv')

data = data.dropna(axis=0)
print(data.isnull().sum())

data.set_index('index', inplace=True)
print(data.head())