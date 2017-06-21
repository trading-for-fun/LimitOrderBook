#ifndef SNAPSHOT_H
#define SNAPSHOT_H
#include <unordered_map>
#include "book.h"
#include "order.h"
#include "message.h"

class Snapshot {

public:
	Snapshot() : last_update_time_(-1) {}
	Snapshot(const vector<string>& stocks) : Snapshot() {
		for (string stock : stocks) {
			AddStock(stock);
		}
	}

	double LastUpdateTime() const { return last_update_time_; }

	unordered_map<string, unique_ptr<Book>>::const_iterator BooksBeginIt() { return books_.begin(); }
	unordered_map<string, unique_ptr<Book>>::const_iterator BooksEndIt() { return books_.end(); }

	Book* GetBook(const string& stock) const{ 
		auto it = books_.find(stock); 
		return it == books_.end() ? nullptr : it->second.get();
	}
	double MaxBidPrice(const string& stock) const { return GetBook(stock)->MaxBidPrice(); }
	double MinAskPrice(const string& stock) const { return GetBook(stock)->MinAskPrice(); }
	int MaxBidVolume(const string& stock) const { return GetBook(stock)->MaxBidVolume(); }
	int MinAskVolume(const string& stock) const { return GetBook(stock)->MinAskVolume(); }

	// returns true if message is relevant and proceeded
	pair<bool, string> Snapshot::ProcessMessage(const Message* pmessage) {
		SetTime(pmessage->Time());
		char messageType = pmessage->Type();
		switch (messageType) {
		case 'A': case 'F': return AddOrder(pmessage->Ref(), pmessage->BSInd(), pmessage->Volume(), pmessage->Stock(), pmessage->Price());
		case 'E': case 'C': case 'X': return DecreaseOrderVolume(pmessage->Ref(), pmessage->Volume());;
		case 'D': return DeleteOrder(pmessage->Ref());
		}
		return {false,""};
	}

private:
	double last_update_time_;
	unordered_map<OrderRef, unique_ptr<Order>> orders_;
	unordered_map<string, unique_ptr<Book>> books_;

	void AddStock(string stock) { books_.insert({ stock,unique_ptr<Book>(new Book()) }); }
	void SetTime(double new_time) { last_update_time_ = new_time; }

	// The following funstions returns true if message is relevant and proceeded
	pair<bool, string> AddOrder(OrderRef order_ref, char BS_ind, int volume, const string& stock, double price) {
		auto pbook = GetBook(stock);
		if (!pbook) return{ false,"" };
		orders_.insert({ order_ref, unique_ptr<Order>(new Order(BS_ind,volume,stock,price,pbook->AddOrder(BS_ind, order_ref, volume, price))) });
		return{ true, stock };
	}
	pair<bool, string> DeleteOrder(OrderRef order_ref) {
		auto it = orders_.find(order_ref);
		if (it == orders_.end()) return{ false,"" };
		string stock = it->second->Stock();
		GetBook(stock)->DecreaseOrderVolume(it->second->BSInd(), it->second->ListIt(), it->second->Volume(), it->second->Price(), it->second->Volume());
		orders_.erase(it);
		return{ true, stock };
	}
	pair<bool, string> DecreaseOrderVolume(OrderRef order_ref, int to_decr) {
		auto it = orders_.find(order_ref);
		if (it == orders_.end()) return{ false,"" };
		string stock = it->second->Stock();
		GetBook(stock)->DecreaseOrderVolume(it->second->BSInd(), it->second->ListIt(), it->second->Volume(), it->second->Price(), to_decr);
		if (it->second->Volume() == to_decr)
			orders_.erase(it);
		else
			it->second->DecreaseVolume(to_decr);
		return{ true, stock };
	}
	pair<bool, string> ReplaceOrder(OrderRef order_ref, OrderRef new_order_ref, int new_order_volume, double new_order_price) {
		auto it = orders_.find(order_ref);
		if (it == orders_.end()) return{ false,"" };
		char BSInd = it->second->BSInd();
		string stock = it->second->Stock();
		DeleteOrder(order_ref);
		AddOrder(new_order_ref, BSInd, new_order_volume, stock, new_order_price);
		return{ true, stock };
	}

};


#endif