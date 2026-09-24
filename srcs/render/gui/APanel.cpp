#include "APanel.hpp"

using namespace render::gui;

APanel::APanel(IGui& gui) : m_gui(gui) {
}

void APanel::open() {
	m_isOpen = true;
}

void APanel::close() {
	m_isOpen = false;
}

void APanel::toggle() {
	m_isOpen = !m_isOpen;
}

bool APanel::isOpen() const {
	return m_isOpen;
}

void APanel::setCaller(const ecs::Entity caller) {
	m_caller = caller;
}
