#pragma once

#include <VkBootstrap.h>
#include "vuk/Context.hpp"
#include <optional>
#include "../examples/utils.hpp"
#include "../examples/glfw.hpp"
#include <string_view>
#include <functional>
#include <string>
#include <stdio.h>
#include <vector>
#include "vuk/RenderGraph.hpp"
#include "vuk/CommandBuffer.hpp"
#include "examples/imgui_impl_glfw.h"

namespace vuk {
	struct BenchRunner;

	struct CaseBase {
		std::string_view label;
		std::vector<std::string_view> subcase_labels;
		std::vector<std::function<RenderGraph(BenchRunner&, vuk::InflightContext&, Query, Query)>> subcases;
		std::vector<std::vector<double>> timings;
		std::vector<std::vector<float>> binned;
		std::vector<uint32_t> last_stage_ran;
		std::vector<uint32_t> runs_required;
		std::vector<double> est_mean;
		std::vector<double> est_variance;
		std::vector<std::pair<double, double>> min_max;
		std::vector<double> mean;
		std::vector<double> variance;
	};

	struct BenchBase {
		std::string_view name;
		std::function<void(BenchRunner&, vuk::InflightContext&)> setup;
		std::function<void(BenchRunner&, vuk::InflightContext&)> gui;
		std::function<void(BenchRunner&, vuk::InflightContext&)> cleanup;
		std::function<CaseBase&(unsigned)> get_case;
		size_t num_cases;
	};

	template<class... Args>
	struct Bench {
		BenchBase base;
		using Params = std::tuple<Args...>;

		struct Case : CaseBase {
			template<class F>
			Case(std::string_view label, F&& subcase_template) : CaseBase{ label } {
				std::apply([this, subcase_template](auto&&... ts) {
					(subcases.emplace_back(
						[=](BenchRunner& runner, vuk::InflightContext& ifc, Query start, Query end) { 
							return subcase_template(runner, ifc, start, end, ts); 
						}), ...);
					(subcase_labels.emplace_back(ts.description), ...);
					timings.resize(sizeof...(Args));
					runs_required.resize(sizeof...(Args));
					mean.resize(sizeof...(Args));
					variance.resize(sizeof...(Args));
					est_mean.resize(sizeof...(Args));
					est_variance.resize(sizeof...(Args));
					last_stage_ran.resize(sizeof...(Args));
					min_max.resize(sizeof...(Args));
					binned.resize(sizeof...(Args));
				}, Params{});
			}
		};

		std::vector<Case> cases;
	};
}

namespace vuk {

	struct BenchRunner {
		VkDevice device;
		VkPhysicalDevice physical_device;
		VkQueue graphics_queue;
		std::optional<Context> context;
		vuk::SwapchainRef swapchain;
		GLFWwindow* window;
		VkSurfaceKHR surface;
		vkb::Instance vkbinstance;
		vkb::Device vkbdevice;
		util::ImGuiData imgui_data;

		Query start, end;
		unsigned current_case = 0;
		unsigned current_subcase = 0;
		unsigned current_stage = 0;

		unsigned num_runs = 0;

		BenchBase* bench;

		BenchRunner();

		void setup() {
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			// Setup Platform/Renderer bindings
			ImGui_ImplGlfw_InitForVulkan(window, true);

			start = context->create_timestamp_query();
			end = context->create_timestamp_query();

			auto ifc = context->begin();
			{
				auto ptc = ifc.begin();
				imgui_data = util::ImGui_ImplVuk_Init(ptc);
				ptc.wait_all_transfers();
			}
			bench->setup(*this, ifc);
		}

		void render();

		void cleanup() {
			context->wait_idle();
			imgui_data.font_texture.view.reset();
			imgui_data.font_texture.image.reset();
			auto ifc = context->begin();
			if (bench->cleanup) {
				bench->cleanup(*this, ifc);
			}

			// this performs cleanups for all inflight frames
			for (auto i = 0; i < vuk::Context::FC; i++) {
				context->begin();
			}
		}

		~BenchRunner() {
			context.reset();
			vkDestroySurfaceKHR(vkbinstance.instance, surface, nullptr);
			destroy_window_glfw(window);
			vkb::destroy_device(vkbdevice);
			vkb::destroy_instance(vkbinstance);
		}

		static BenchRunner& get_runner() {
			static BenchRunner runner;
			return runner;
		}
	};
}

namespace util {
	struct Register {
		template<class... Args>
		Register(vuk::Bench<Args...>& x) {
			vuk::BenchRunner::get_runner().bench = &x.base;
			vuk::BenchRunner::get_runner().bench->get_case = [&x](unsigned i) -> vuk::CaseBase& { return x.cases[i]; };
			vuk::BenchRunner::get_runner().bench->num_cases = x.cases.size();
		}
	};
}

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define REGISTER_BENCH(x) util::Register MACRO_CONCAT(_reg_, __LINE__) (x)
