#pragma once

#include <atomic>
#include <memory>

namespace concurrency {
	class CancelToken {
	private:
		std::shared_ptr<std::atomic<bool>> m_cancelled = std::make_shared<std::atomic<bool>>(false);

	public:
		void cancel() const {
			m_cancelled->store(true);
		}

		[[nodiscard]] bool isCancelled() const {
			return m_cancelled->load();
		}
	};

	/** Cancels and forgets the token stored under the key. Returns whether there was one. */
	template<typename TokenMap>
	bool cancelAndErase(TokenMap& tokens, const typename TokenMap::key_type& key) {
		const auto token = tokens.find(key);
		if (token == tokens.end())
			return false;

		token->second.cancel();
		tokens.erase(token);
		return true;
	}
}
