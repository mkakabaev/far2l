#include "OpChangeMode.h"
#include "../UI/Activities/ConfirmChangeMode.h"
#include "../UI/Activities/SimpleOperationProgress.h"

OpChangeMode::OpChangeMode(std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		std::shared_ptr<WhatOnErrorState> &wea_state, struct PluginPanelItem *items, int items_count)
	:
	OpBase(0, base_host, base_dir, wea_state),
	_recurse(false),
	_mode_set(0),
	_mode_clear(0)
{
	bool has_dirs = false;
	mode_t mode_all = 06777, mode_any = 0;
	for (int i = 0; i < items_count; ++i) {
		if (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			has_dirs = true;
			mode_all = 0;
			mode_any = 06777;
			break;
		}
		mode_all&= items[i].FindData.dwUnixMode;
		mode_any|= (items[i].FindData.dwUnixMode & 06777);
	}

	if (!ConfirmChangeMode(base_dir, has_dirs, mode_all, mode_any).Ask(_recurse, _mode_set, _mode_clear)) {
		throw AbortError();
	}

	_enumer = std::make_shared<Enumer>(_entries, _base_host, _base_dir, items, items_count, true, _state, _wea_state);
}

void OpChangeMode::Do()
{
	if (!StartThread()) {
		;

	} else if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress p(SimpleOperationProgress::K_CHANGEMODE, _base_dir, _state);
		p.Show();
		WaitThread();
	}
}


void OpChangeMode::Process()
{
	_enumer->Scan(_recurse);
	const auto &items = _enumer->Items();
	for (const auto &path : items) {
		ChangeModeOfPath(path);
		ProgressStateUpdate psu(_state);
		_state.stats.count_complete++;
	}
}

void OpChangeMode::ChangeModeOfPath(const std::string &path)
{
	WhatOnErrorWrap<WEK_CHMODE>(_wea_state, _state, _base_host.get(), path,
		[&] () mutable
		{
			mode_t prev_mode = _base_host->GetMode(path);
			mode_t new_mode = prev_mode;
			new_mode&= (~_mode_clear);
			new_mode|= (_mode_set);
			if (new_mode != prev_mode) {
				_base_host->SetMode(path.c_str(), new_mode);
			}
		}
	);
}

