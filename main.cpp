#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <map>
#include <iterator>

volatile int common = 0;

std::map<int, int> coins
{
	{1, 100}, {2, 100}, {5, 100}, {10, 10},
	{25, 10}, {50, 10}, {100, 10}
};

int TestAndSet(volatile int* common)
{
	int tmp = *common;
	*common = 1;

	return tmp;
}

int GenerateRandomCoin()
{
	std::random_device rd;
	std::mt19937 generator(rd());
	std::uniform_int_distribution<int> distribution(1, coins.size() - 1);
	auto rand = distribution(generator);

	auto it = coins.begin();
	std::advance(it, rand);

	std::cout << "Rand number is: " << it->first << std::endl;

	return it->first;
}

bool isExchangable(std::atomic<int>& index, int exchangeNum)
{
	std::cout << "-----------" << std::endl;

	if (exchangeNum == -1) std::exit(1);

	if (coins.find(exchangeNum) == coins.end())
	{
		std::cout << "ERROR: Invalid nominal" << std::endl;
		return false;
	}

	if (exchangeNum >= index)
	{
		std::cout << "ERROR: Value of exchanged coin are greater or equal than given" << std::endl;
		return false;
	}

	if (index % exchangeNum != 0)
	{
		std::cout << "ERROR: Can't exchange " << index << " with " << exchangeNum << std::endl;
		return false;
	}

	if (coins[exchangeNum] <= 0 || coins[exchangeNum] < index / exchangeNum)
	{
		std::cout << "ERROR: Not enough coins of this nominal" << std::endl;
		return false;
	}

	return true;
}

void* IdentifyCoin(std::atomic_int& index, std::atomic_bool& isReadyToExchange)
{
	while (1)
	{
		while (isReadyToExchange);
		while (TestAndSet(&common) == 1);

		index = GenerateRandomCoin();
		coins[index]++;

		isReadyToExchange = true;
		common = 0;

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return nullptr;
}

void* ExchangeCoin(std::atomic_int& index, std::atomic_bool& isReadyToExchange)
{
	int exchange;

	while (1)
	{
		// in case IdentifyCoin isn't called first => coin can't be exchanged
		while (!isReadyToExchange);
		while (TestAndSet(&common) == 1);

		std::cout << "Choose nominal to exchange: ";
		std::cin >> exchange;

		while (!isExchangable(index, exchange))
		{
			std::cout << "Choose nominal to exchange (-1 to finish): ";
			std::cin >> exchange;
		}

		std::cout << "-----------" << std::endl;

		coins[exchange] -= index / exchange;

		for (auto it = coins.begin(); it != coins.end(); ++it)
		{
			std::cout << it->first << " " << it->second << " " << std::endl;
		}

		std::cout << "-----------" << std::endl;

		isReadyToExchange = false;
		common = 0;

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	return nullptr;
}

int main()
{
	for (auto it = coins.begin(); it != coins.end(); ++it)
	{
		std::cout << it->first << " " << it->second << " " << std::endl;
	}
	std::cout << "-----------" << std::endl;

	std::atomic_int index;
	std::atomic_bool isReadyToExchange = false;

	std::vector<std::thread> threads;
	threads.emplace_back(IdentifyCoin, std::ref(index), std::ref(isReadyToExchange));
	threads.emplace_back(ExchangeCoin, std::ref(index), std::ref(isReadyToExchange));

	for (auto& thread : threads)
	{
		thread.join();
	}

	return 0;
}
